#!/bin/sh

# Launch a RethinkDB backup script using Python 2

for python in python python2 python27 python2.7 python26 python2.6; do
    ver="`$python -c 'import sys; print(sys.version_info.major)' 2>&1`"
    if test "$ver" = 2 ; then
        exec $python "$0.py" "$@"
    fi
done

name=`basename $0`
echo "$name: error: Python 2 not found" >&2
echo "$name requires both Python 2 and the rethinkdb python module" >&2
exit 1
