// Author: Shu-chun Weng (scweng@cs.yale.edu)
// Author: Alexander Thomson (thomson@cs.yale.edu)
// Author: Kun Ren (kun@cs.yale.edu)

#include "common/configuration.h"

#include <netdb.h>
#include <netinet/in.h>

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "common/utils.h"
#include "applications/tpcc.h"

using std::string;

Configuration::Configuration(int node_id, const string& filename)
    : this_node_id(node_id) {
  if (ReadFromFile(filename))  // Reading from file failed.
    exit(0);
}

// TPCC-aware partitioning based on (X shards per warehouse) and W warehouses.
// Node layout assumption: nodes are arranged as shard-major blocks of size W.
// Node id for (warehouse w, shard i) := (i % X) * W + (w % W), then modulo N.
// Key placement:
//  - Warehouse and YTD and History: shard i = 0 (warehouse owner shard)
//  - District/Customer/Order/NewOrder/OrderLine: shard i = district_id % X
//  - Stock: shard i = item_id % X
//  - Item: shard i = item_id % X (no warehouse; mapped to shard group 0)
int Configuration::LookupPartition(const Key& key) const {
  const int N = static_cast<int>(all_nodes.size());
  if (N <= 0) return 0;
  int TOTAL_WAREHOUSES = (int) N / DISTRICTS_PER_WAREHOUSE;
  // Compute W and X.
  int W;
  if (TOTAL_WAREHOUSES > 0) {
    W = TOTAL_WAREHOUSES;
  } else {
    W = WAREHOUSES_PER_NODE * N;
  }
  if (W <= 0) W = 1;
  int X = N / W;
  if (X <= 0) X = 1;

  // Helpers to parse ints out of key substrings.
  auto parse_int_from = [](const string& s, size_t start) -> int {
    size_t i = start;
    while (i < s.size() && isdigit(s[i])) i++;
    if (i == start) return 0;
    return atoi(s.substr(start, i - start).c_str());
  };

  if (!key.empty() && key[0] == 'w') {
    // Parse warehouse id after 'w'.
    int w = 0;
    {
      size_t pos_w = 1;
      w = parse_int_from(key, pos_w);
      if (w < 0) w = 0;
    }

    // Look for district id 'd' and item id 'i' in stock keys.
    int d = -1;
    int item = -1;
    size_t pos_d = key.find('d');
    if (pos_d != string::npos) {
      d = parse_int_from(key, pos_d + 1);
    }
    size_t pos_i = key.find('i');
    if (pos_i != string::npos) {
      item = parse_int_from(key, pos_i + 1);
    }

    int shard = 0;
    // Stock key includes 's' and 'i'.
    if (key.find('s') != string::npos && item >= 0) {
      shard = item % X;
    } else if (d >= 0) {
      // District, customer, order/neworder/orderline derive shard from district.
      shard = d % X;
    } else {
      // Warehouse, YTD, history default to shard 0 for this warehouse.
      shard = 0;
    }

    int node = ((w % W) * X + (shard % X)) % N;
    return node;
  }

  if (!key.empty() && key[0] == 'i') {
    // Item-only keys: distribute by residue across shards; map to shard group 0.
    int item = parse_int_from(key, 1);
    int shard = ((item < 0) ? 0 : (item % X));
    int node = (shard % X) % N;
    return node;
  }

  // Fallback: hash by numeric value modulo N.
  return StringToInt(key) % N;
}

bool Configuration::WriteToFile(const string& filename) const {
  FILE* fp = fopen(filename.c_str(), "w");
  if (fp == NULL)
      return false;
  for (map<int, Node*>::const_iterator it = all_nodes.begin();
       it != all_nodes.end(); ++it) {
    Node* node = it->second;
    fprintf(fp, "node%d=%d:%d:%d:%s:%d\n",
            it->first,
            node->replica_id,
            node->partition_id,
            node->cores,
            node->host.c_str(),
            node->port);
  }
  fclose(fp);
  return true;
}

int Configuration::ReadFromFile(const string& filename) {
  char buf[1024];
  FILE* fp = fopen(filename.c_str(), "r");
  if (fp == NULL) {
    printf("Cannot open config file %s\n", filename.c_str());
    return -1;
  }
  char* tok;
  // Loop through all lines in the file.
  while (fgets(buf, sizeof(buf), fp)) {
    // Seek to the first non-whitespace character in the line.
    char* p = buf;
    while (isspace(*p))
      ++p;
    // Skip comments & blank lines.
    if (*p == '#' || *p == '\0')
      continue;
    // Process the rest of the line, which has the format "<key>=<value>".
    char* key = strtok_r(p, "=\n", &tok);
    char* value = strtok_r(NULL, "=\n", &tok);
    ProcessConfigLine(key, value);
  }
  fclose(fp);
  return 0;
}

void Configuration::ProcessConfigLine(char key[], char value[]) {
  if (strncmp(key, "node", 4) != 0) {
#if VERBOSE
    printf("Unknown key in config file: %s\n", key);
#endif
  } else {
    Node* node = new Node();
    // Parse node id.
    node->node_id = atoi(key + 4);

    // Parse additional node addributes.
    char* tok;
    node->replica_id   = atoi(strtok_r(value, ":", &tok));
    node->partition_id = atoi(strtok_r(NULL, ":", &tok));
    node->cores        = atoi(strtok_r(NULL, ":", &tok));
    const char* host   =      strtok_r(NULL, ":", &tok);
    node->port         = atoi(strtok_r(NULL, ":", &tok));

    // Translate hostnames to IP addresses.
    string ip;
    {
      struct hostent* ent = gethostbyname(host);
      if (ent == NULL) {
        ip = host;
      } else {
        uint32_t n;
        char buf[32];
        memmove(&n, ent->h_addr_list[0], ent->h_length);
        n = ntohl(n);
        snprintf(buf, sizeof(buf), "%u.%u.%u.%u",
            n >> 24, (n >> 16) & 0xff,
            (n >> 8) & 0xff, n & 0xff);
        ip = buf;
      }
    }
    node->host = ip;

    all_nodes[node->node_id] = node;
  }
}

int Configuration::ThisNodeCoreStart() const {
  // Compute start index by summing cores of nodes on the same host
  // with smaller node_id than this_node_id. This creates disjoint
  // contiguous ranges per host.
  if (all_nodes.find(this_node_id) == all_nodes.end()) return 0;
  const Node* self = all_nodes.find(this_node_id)->second;
  const string& host = self->host;
  int start = 0;
  for (map<int, Node*>::const_iterator it = all_nodes.begin();
       it != all_nodes.end(); ++it) {
    if (it->first == this_node_id) continue;
    const Node* other = it->second;
    if (other->host == host && it->first < this_node_id) {
      start += other->cores;
    }
  }
  return start;
}

int Configuration::ThisNodeCores() const {
  if (all_nodes.find(this_node_id) == all_nodes.end()) return 1;
  return all_nodes.find(this_node_id)->second->cores;
}

