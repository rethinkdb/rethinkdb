#!/bin/sh
set -e

case "$1" in
    remove|upgrade|deconfigure|failed-upgrade)
        invoke-rc.d rethinkdb stop || true;
        ;;

    *)
        echo "Warning: RethinkDB prerm script called with unknown set of arguments: $*" >&2 ;;
esac

