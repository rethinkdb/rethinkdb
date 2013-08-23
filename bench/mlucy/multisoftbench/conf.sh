# This is a multi-benchmark that compares different things.
SUBCONF=subconf
RUNS=1
NAME='$SERVERS-$SERVER_INSTANCES'
CONFS=`awk '{print "export SERVERS="$1"; export SERVER_INSTANCES="$2}' <<EOF
1 1
1 2
1 3
1 8
2 1
2 2
2 4
EOF`
