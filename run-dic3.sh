#!/bin/bash
if [ $# -lt 1 ]; then
    echo "Usage: $0 <warehouse_number>"
    exit 1
fi
warehouse_number=$1

cd src
make -j 
cd ..

rm -f ~/.ssh/known_hosts

./docker/build_docker.sh

cd ./dic3/scripts
./setup_clusters.sh $warehouse_number
cd ../..


bin/deployment/cluster -c deploy-run.conf -p src/deployment/portfile -d bin/deployment/db t 0