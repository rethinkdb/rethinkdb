# This is a benchmark configuraiton file.  It's just a shell script,
# so write whatever you want as long as you export the right
# variables.

# Table configuration.
TABLES=1
TABLE_SHARDS=8
# Put any admin commands you want here.
TABLE_CONF=`cat <<"EOF"
set durability $table --soft
EOF`

# Number of clients.
CLIENTS=3
# Be sure to mimic this quoting.
CLIENT_MAP="grep -E '^(read|write)' | awk '{print \$5}'"
CLIENT_REDUCE="awk '{a+=\$0} END {print a}'"

# Where to find `rethinkdb` and `rethinkdb_web_assets`.
RETHINKDB_DIR=`cd -P -- "$(dirname $0)" && pwd -P`/../../build/release

# Where to stage from on the remote server.
CLUSTER=local # Change this to run on ec2.
STAGING="/mnt/bench"

