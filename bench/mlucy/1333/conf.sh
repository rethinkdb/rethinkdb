# This is a benchmark configuraiton file.  It's just a shell script,
# so write whatever you want as long as you export the right
# variables.

# Number of servers, and number of RethinkDB instances per server.
SERVERS=1
SERVER_INSTANCES=1
SERVER_MACHINE=m1.medium # EC2 only
SERVER_OPTS="-d ../.persist/1333/data"

# Table configuration.
TABLES=0 # Pre-existing cluster
# TABLE_SHARDS=8
# Put any admin commands you want here.
# TABLE_CONF=`cat <<"EOF"
# set durability $table --soft
# EOF`

# Number of clients.
CLIENTS=1
CLIENT_MACHINE=m1.small # EC2 only
# Be sure to mimic this quoting.
# CLIENT_MAP="grep -E '^(read|write)' | awk '{print \$5}'"
# CLIENT_REDUCE="awk '{a+=\$0} END {print a}'"

# Where to find `rethinkdb` and `rethinkdb_web_assets`.
RETHINKDB_DIR=~/rethinkdb/build/release

# Where to stage from on the remote server.
CLUSTER=local
RUN_AT_SLEEP=10
STAGING="/mnt/ssd/bench"
