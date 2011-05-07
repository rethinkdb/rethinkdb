#!/bin/bash
if [ $# -ne 2 ]
then
    lower=10000
    upper=10100
else
    lower="$1"
    upper="$2"
fi
cat <(netstat -an | grep -i tcp.*listen | sed 's/^.*:\([0-9]*\)\ .*$/\1/' | sort -nu) <(seq "$lower" "$upper") <(seq "$lower" "$upper") | sort | uniq -c | grep "      2" | sed 's/^\ \ \ \ \ \ 2\ \([0-9]*\)$/\1/' | head -n 1
