#!/usr/bin/env python
# Copyright 2015 RethinkDB, all rights reserved.

import os, pprint, sys, time, traceback

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
op["num_rows"] = vcoptparse.IntFlag("--num-rows", 50000)
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)
_, command_prefix, server_options = scenario_common.parse_mode_flags(opts)

r = utils.import_python_driver()

utils.print_with_time("Starting cluster of two servers")
with driver.Cluster(initial_servers=['a', 'b'], console_output=True, command_prefix=command_prefix, extra_options=server_options) as cluster:
    server_a = cluster[0]
    server_b = cluster[1]
    conn = r.connect(host=server_a.host, port=server_a.driver_port)

    utils.print_with_time("Creating a table")
    num_shards = 16
    r.db_create("test").run(conn)
    r.db("rethinkdb").table("table_config").insert({
        "name": "test",
        "db": "test",
        "shards": [{"primary_replica": "a", "replicas": ["a"]}] * num_shards
        }).run(conn)
    r.table("test").wait().run(conn)

    utils.print_with_time("Inserting %d documents" % opts["num_rows"])
    docs_so_far = 0
    while docs_so_far < opts["num_rows"]:
        chunk = min(opts["num_rows"] - docs_so_far, 1000)
        res = r.table("test").insert(r.range(chunk).map({
            "value": r.row,
            "padding": "x" * 100
            }), durability="soft").run(conn)
        assert res["inserted"] == chunk
        docs_so_far += chunk
        utils.print_with_time("Progress: %d/%d" % (docs_so_far, opts["num_rows"]))

    utils.print_with_time("Beginning replication to second server")
    r.table("test").config().update({
        "shards": [{"primary_replica": "a", "replicas": ["a", "b"]}] * num_shards
        }).run(conn)
    status = r.table("test").status().run(conn)
    assert status["status"]["ready_for_writes"], status
    assert not status["status"]["all_replicas_ready"], status

    utils.print_with_time("Waiting a few seconds for backfill to get going")
    time.sleep(2)

    utils.print_with_time("Shutting down both servers")
    server_a.check_and_stop()
    server_b.check_and_stop()

    utils.print_with_time("Restarting both servers")
    server_a.start()
    server_b.start()
    conn_a = r.connect(host=server_a.host, port=server_a.driver_port)
    conn_b = r.connect(host=server_b.host, port=server_b.driver_port)

    utils.print_with_time("Checking that table is available for writes")
    try:
        r.table("test").wait(wait_for="ready_for_writes", timeout=10).run(conn_a)
    except r.ReqlRuntimeError, e:
        status = r.db("rethinkdb").table("_debug_table_status").nth(0).run(conn_a)
        pprint.pprint(status)
        raise
    try:
        r.table("test").wait(wait_for="ready_for_writes", timeout=3).run(conn_b)
    except r.ReqlRuntimeError, e:
        status = r.db("rethinkdb").table("_debug_table_status").nth(0).run(conn_b)
        pprint.pprint(status)
        raise

    utils.print_with_time("Making sure the backfill didn't end")
    status = r.table("test").status().run(conn_a)
    assert not status["status"]["all_replicas_ready"], status

    utils.print_with_time("Cleaning up")
utils.print_with_time("Done.")
