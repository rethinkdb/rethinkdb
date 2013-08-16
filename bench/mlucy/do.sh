#!/bin/bash
. `cd -P -- "$(dirname $0)" && pwd -P`/init.sh
dir=`mktemp -d`
echo "Staging from $dir..." >&2
cd $dir

eval "$@"
