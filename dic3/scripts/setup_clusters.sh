#!/bin/bash

nodes=${1:-4}
Re=${2:-2} # If Re == 1, stop containers instead

num=$((nodes))
sudo ./unset_ovs.sh $num
./kill_containers.sh
sleep 2
if [ $Re -eq 1 ]; then
	exit 0
fi

./start_containers.sh $num
sleep 2
sudo ./set_ovs.sh $num

PREFIX="192.168.20."
dir=$(dirname "$0")

confFile="${dir}/../../deploy-run.conf"
rm -f ${confFile}
touch ${confFile}

# Output header for clarity
echo "# Node<id>=<replica>:<partition>:<cores>:<host>:<port>" >> ${confFile}
CORES=16
REPLICA=0  # Single replica

for (( nodeId=0; nodeId<nodes; nodeId++ )); do
  addr=${PREFIX}$((nodeId + 2))
  for (( districtId=0; districtId < 10; districtId++ )); do
    partition=$((nodeId * 10 + districtId))
    port=$((10000 + districtId))
    echo "node${partition}=${REPLICA}:${partition}:${CORES}:${addr}:${port}" >> ${confFile}
  done
done

# for (( nodeId=0; nodeId<nodes; nodeId++ )); do
#   addr=$PREFIX$((nodeId + 2))
#   scp -o StrictHostKeyChecking=no ${confFile} root@${addr}:/root/ShardDB/deploy-run.conf
# done
# scp -o StrictHostKeyChecking=no ${tomlFile} root@${ClientAddr}:/root/ShardDB/server/node.toml


./add_latency_all.sh "$nodes"
