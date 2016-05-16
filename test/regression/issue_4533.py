#!/usr/bin/env python
# Copyright 2015-2016 RethinkDB, all rights reserved.

import os, pprint, sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, utils

r = utils.import_python_driver()

utils.print_with_time("Starting one server")
with driver.Process() as process:
    utils.print_with_time("Establishing ReQl connections")
    conn = r.connect(host=process.host, port=process.driver_port)

    utils.print_with_time("Creating a table")
    r.db_create("test").run(conn)
    table_uuid = r.table_create("test").run(conn)["config_changes"][0]["new_val"]["id"]
    r.table("test").wait(wait_for="all_replicas_ready").run(conn)

    utils.print_with_time("Creating an index")
    r.table("test").index_create("test").run(conn)
    r.table("test").index_wait().run(conn)
    old_indexes = r.table("test").index_list().run(conn)

    utils.print_with_time("Modifying table configuration through `rethinkdb.table_config`")
    r.db("rethinkdb").table("table_config").get(table_uuid).update(
        {"durability": "soft", "write_acks": "single"}
    ).run(conn)

    utils.print_with_time("Verifying configuration")
    new_config = r.table("test").config().run(conn)
    assert new_config["durability"] == "soft", new_config["durability"]
    assert new_config["write_acks"] == "single", new_config["write_acks"]
    new_indexes = r.table("test").index_list().run(conn)
    assert new_indexes == old_indexes, (new_indexes, old_indexes)

    utils.print_with_time("Modifying table configuration through `.reconfigure`")
    r.table("test").reconfigure(shards=2, replicas=1).run(conn)

    utils.print_with_time("Verifying configuration")
    new_config = r.table("test").config().run(conn)
    assert new_config["durability"] == "soft", new_config["durability"]
    assert new_config["write_acks"] == "single", new_config["write_acks"]
    new_indexes = r.table("test").index_list().run(conn)
    assert new_indexes == old_indexes, (new_indexes, old_indexes)

    utils.print_with_time("Cleaning up")
utils.print_with_time("Done.")
