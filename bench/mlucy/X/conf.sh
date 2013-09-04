# This is a benchmark configuraiton file.  It's just a shell script,
# so write whatever you want as long as you export the right
# variables.

SERVERS=2
SERVER_INSTANCES=2
SERVER_OPTS=" -d .persist/dataX"
SERVER_STAGING=/mnt/ssd

TABLES=1
TABLE_SHARDS=4
TABLE_CONF=`cat <<"EOF"
set replicas $table 2
set acks $table 2
EOF`

CLIENTS=2
CLIENT_STAGING=/home/ubuntu/client
