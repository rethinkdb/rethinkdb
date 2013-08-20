#!/bin/bash
set -e
set -o nounset

table=`head -1 "$1"`
hostflags=`<"$2" awk '{print " --host="$0}'`
`dirname "$0"`/stress.py --timeout 15 --table ${table}_db.$table $hostflags
