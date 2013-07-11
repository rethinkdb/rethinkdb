#!/bin/sh -e
### BEGIN INIT INFO
# Provides:          firstrun
# Required-Start:    $remote_fs
# Required-Stop:
# Default-Start:     1 2 3 4 5
# Default-Stop:
# Short-Description: Prepare the RethinkDB instance for its first run
### END INIT INFO
#
# Restore /etc/resolv.conf if the system crashed before the ppp link
# was shut down.

start () {
    if ! test -e /etc/nginx/ssl.key; then
        echo "Generating a new self-signed SSL certificate."
        openssl req -new -newkey rsa:4096 -days 1024 -nodes -x509 \
                -subj "/C=US/ST=California/L=MountainView/O=IT/CN=localhost" \
                -keyout /etc/nginx/ssl.key -out /etc/nginx/ssl.crt
    fi
    cd /opt/ami-setup
    echo "Launching first-run web app."
    twistd --pidfile=/opt/ami-setup/twisted.pid web --wsgi=firstrun_web.application --port=8888
}

stop () {
    kill `cat /opt/ami-setup/twisted.pid`
}

case "$1" in
    start) start;;
    stop) stop;;
    restart) stop; start;;
    *) ;;
esac
