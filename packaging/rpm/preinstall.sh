#!/bin/sh
# Copyright 2026 RethinkDB, all rights reserved.
#
# RPM %pre scriptlet. Create the system account used by the server.

set -eu

if ! getent group rethinkdb >/dev/null 2>&1; then
    groupadd -r rethinkdb
fi

if ! getent passwd rethinkdb >/dev/null 2>&1; then
    useradd --system --no-create-home --gid rethinkdb --shell /sbin/nologin \
        --comment "RethinkDB Daemon" rethinkdb
fi
