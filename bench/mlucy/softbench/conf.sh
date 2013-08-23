# This is a benchmark configuraiton file.  It's just a shell script,
# so write whatever you want as long as you export the right
# variables.

# Number of servers, and number of RethinkDB instances per server.
SERVERS=2
SERVER_INSTANCES=4
SERVER_MACHINE=m1.medium # EC2 only
#TODO: SERVER_OPTS=""

# Table configuration.
TABLES=1
TABLE_SHARDS=16
# Put any admin commands you want here.
TABLE_CONF=`cat <<"EOF"
set durability $table --soft
EOF`

# Number of clients.
CLIENTS=3
CLIENT_MACHINE=m1.small # EC2 only
# Be sure to mimic this quoting.
CLIENT_MAP="grep -E '^(read|write)' | awk '{print \$5}'"
CLIENT_REDUCE="awk '{a+=\$0} END {print a}'"

# Where to find `rethinkdb` and `rethinkdb_web_assets`.
RETHINKDB_DIR=~/rethinkdb/build/release

# Where to stage from on the remote server.
CLUSTER=local
STAGING="/mnt/bench"

