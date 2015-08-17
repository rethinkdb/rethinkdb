#!/bin/bash

if [ $# -ne 2 ]
then
  echo "Usage: `basename $0` drive scheduler"
  exit 65
fi

DRIVE=$1
SCHEDULER=$2

echo $SCHEDULER > /sys/block/$DRIVE/queue/scheduler
