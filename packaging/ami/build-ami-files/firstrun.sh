
exec >> firstrun.log 2>&1

password=$1

# Issue #1069
mkdir -p /var/run/rethinkdb
chmod a+rx /var/run/rethinkdb

# nginx config
htpasswd -b -c /etc/nginx/htpasswd rethinkdb "$password"
chmod a+r /etc/nginx/htpasswd
chown ubuntu@ubuntu /etc/nginx/htpasswd
cp nginx.conf /etc/nginx/sites-available/default
openssl req -new -newkey rsa:4096 -days 1024 -nodes -x509 -subj "/" -keyout /etc/nginx/ssl.key  -out /etc/nginx/ssl.crt

# rethinkdb config
cp rethinkdb.conf /etc/rethinkdb/instances.d/default.conf

# reload nginx
nginx -s reload

# start rethinkdb
service firstrun stop
update-rc.d firstrun disable
service rethinkdb restart

# set api key
sleep 3
rethinkdb admin --join localhost:29015 set auth "$password"
