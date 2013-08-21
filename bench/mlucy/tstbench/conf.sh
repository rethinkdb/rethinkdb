# This is a test/example configuration file for EC2 benchmarking.
# It's just a shell script, so write whatever you want as long as you
# export the right variables.

# Number of servers, and number of RethinkDB instances per server.
SERVERS=2
SERVER_INSTANCES=2
SERVER_MACHINE=m1.medium
#TODO: SERVER_OPTS=""

# Table configuration.
TABLES=1
TABLE_SHARDS=8
# Put any admin commands you want here.
TABLE_CONF=`cat <<"EOF"
set replicas $table 2
set acks $table 2
EOF`

# Number of clients.
CLIENTS=8
CLIENT_MACHINE=m1.small
# Be sure to mimic this quoting.
CLIENT_MAP="grep ^read | awk '{print \$5}'"
CLIENT_REDUCE="awk '{a+=\$0} END {print a}'"

# Where to find `rethinkdb` and `rethinkdb_web_assets`.
RETHINKDB_DIR=~/rethinkdb/build/release

# Where to stage from on the remote server.  You should only need to
# change this if something on EC2 changes.
STAGING="/mnt/bench"

