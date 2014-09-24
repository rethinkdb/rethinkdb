#!/bin/sh
set -e

case "$1" in
    remove|purge|upgrade|disappear|failed-upgrade|abort-install|abort-upgrade)

        update-rc.d -f rethinkdb remove || true;

        if [ "$1" = purge ]; then
            deluser --system --quiet rethinkdb || true
            delgroup --system --quiet rethinkdb || true
        fi
        ;;

    *)
        echo "Warning: RethinkDB postrm script called with unknown set of arguments: $*" >&2 ;;
esac
