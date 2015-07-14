#!/usr/bin/env python
# Copyright 2015 RethinkDB, all rights reserved.

from __future__ import print_function

import os, pprint, sys, time, traceback

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, utils

r = utils.import_python_driver()

print("Starting one server (%.2fs)" % (time.time() - startTime))
with driver.Process() as process:
    print("Establishing ReQl connections (%.2fs)" % (time.time() - startTime))
    conn = r.connect(host=process.host, port=process.driver_port)

    print("Creating a table (%.2fs)" % (time.time() - startTime))
    r.db_create("test").run(conn)
    table_uuid = r.table_create("test").run(conn)["config_changes"][0]["new_val"]["id"]
    r.table("test").wait().run(conn)

    print("Creating an index (%.2fs)" % (time.time() - startTime))
    r.table("test").index_create("test").run(conn)
    r.table("test").index_wait().run(conn)
    old_indexes = r.table("test").index_list().run(conn)

    print("Modifying table configuration (%.2fs)" % (time.time() - startTime))
    r.db("rethinkdb").table("table_config").get(table_uuid).update({"durability": "soft"}).run(conn)

    print("Verifying index (%.2fs)" % (time.time() - startTime))
    new_indexes = r.table("test").index_list().run(conn)
    assert new_indexes == old_indexes, (new_indexes, old_indexes)

    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))
