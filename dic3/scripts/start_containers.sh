#!/bin/bash
set -e

nodes=${1:-4}
IMGNAME="icnode"
PREFIX="icnode"

CPUS_PER_CONTAINER=4

DFILE=dockers.txt
rm -rf $DFILE

regions=$((nodes - 1))
for idx in `seq 1 ${regions}`; do
	CPUID=$((($idx - 1)*$CPUS_PER_CONTAINER))
  CPUIDS=$CPUID
  for jdx in `seq 1 $(($CPUS_PER_CONTAINER-1))`; do
    CPUIDS="$CPUIDS,$(($CPUID+$jdx))"
  done
  echo "${CPUIDS}"
	#docker run -d --publish-all=true --cap-add=SYS_ADMIN --cap-add=NET_ADMIN --security-opt seccomp:unconfined --cpuset-cpus=$CPUIDS --name=$PREFIX$idx $IMGNAME tail -f /dev/null 2>&1 >> $DFILE
	docker run -d --publish-all=true --cap-add=SYS_ADMIN --cap-add=NET_ADMIN --security-opt seccomp:unconfined --name=$PREFIX$idx $IMGNAME tail -f /dev/null 2>&1 >> $DFILE
done

CPUID=$(($regions*$CPUS_PER_CONTAINER))
CPUIDS=$CPUID
for jdx in `seq 1 $(($CPUS_PER_CONTAINER-1))`; do
  CPUIDS="$CPUIDS,$(($CPUID+$jdx))"
done
echo "${CPUIDS}"
#docker run -d --publish-all=true --cap-add=SYS_ADMIN --cap-add=NET_ADMIN --security-opt seccomp:unconfined --cpuset-cpus=$CPUIDS --name=$PREFIX$nodes $IMGNAME tail -f /dev/null 2>&1 >> $DFILE
docker run -d --publish-all=true --cap-add=SYS_ADMIN --cap-add=NET_ADMIN --security-opt seccomp:unconfined --name=$PREFIX$nodes $IMGNAME tail -f /dev/null 2>&1 >> $DFILE

while read ID; do
	docker exec $ID "/usr/sbin/sshd"
done < $DFILE