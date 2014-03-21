#!/bin/sh
set -e

create_rethinkdb_user () {
    adduser --system --quiet --group --no-create-home rethinkdb || true
}

rethinkdb_enable_init () {
    update-rc.d rethinkdb defaults
    invoke-rc.d rethinkdb start
    if [ "`find /etc/rethinkdb/instances.d -maxdepth 1 -type f -name '*.conf'`" = "" ]; then
        echo The RethinkDB startup service is installed but disabled. To enable it,
        echo follow the instructions in the guide located at http://www.rethinkdb.com/docs/guides/startup/
    fi
}

case "$1" in
    configure)
        create_rethinkdb_user
        rethinkdb_enable_init ;;

    abort-upgrade|abort-remove|abort-deconfigure)
        rethinkdb_enable_init ;;

    *)
        echo "Warning: RethinkDB postinst script called with unknown set of arguments: $*" >&2 ;;
esac

