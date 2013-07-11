set -x

exec >> firstrun.log 2>&1

password=$1
joins=$2

# Issue #1069
mkdir -p /var/run/rethinkdb
chmod a+rx /var/run/rethinkdb

# nginx config
htpasswd -b -c /etc/nginx/htpasswd rethinkdb "$password"
chmod a+r /etc/nginx/htpasswd
chown ubuntu:ubuntu /etc/nginx/htpasswd
cp nginx.conf /etc/nginx/sites-available/default

# rethinkdb config
( cat rethinkdb.conf;
  for join in $joins; do
      echo join = $join
  done
) > /etc/rethinkdb/instances.d/default.conf

# create the data directory
parent_directory=/var/lib/rethinkdb/default
mkdir -p $parent_directory
chown rethinkdb:rethinkdb $parent_directory
sudo -u rethinkdb rethinkdb create -d $parent_directory/data

# reload nginx
nginx -s reload

# start rethinkdb
service firstrun stop
update-rc.d firstrun disable
service rethinkdb restart

# set api key
sleep 3
rethinkdb admin --join localhost:29015 set auth "$password"

# create a test database
test -z "$joins" && rethinkdb admin --join localhost:29015 create database test
