#!/bin/bash

if [ $# -ne 1 ]
then
  echo "Usage: `basename $0` numberOfCores"
  exit 65
fi

NUMBER_OF_CORES=$1

CURRENT_CORE=0
for CORE in /sys/devices/system/cpu/cpu? /sys/devices/system/cpu/cpu??; do
    if [ "$NUMBER_OF_CORES" -gt "$CURRENT_CORE" ]; then
        echo 1 > $CORE/online
    else
        echo 0 > $CORE/online
    fi
    let CURRENT_CORE=CURRENT_CORE+1
done
