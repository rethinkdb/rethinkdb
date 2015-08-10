#!/usr/bin/env python
# Copyright 2015 RethinkDB, all rights reserved.

from __future__ import print_function

import os, pprint, sys, time, traceback

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
op["num_rows"] = vcoptparse.IntFlag("--num-rows", 50000)
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)

r = utils.import_python_driver()

print("Starting cluster of two servers (%.2fs)" % (time.time() - startTime))
with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    files_a = driver.Files(metacluster, "a", db_path="a_data",
        command_prefix=command_prefix, console_output=True)
    files_b = driver.Files(metacluster, "b", db_path="b_data",
        command_prefix=command_prefix, console_output=True)
    server_a_1 = driver.Process(cluster, files_a,
        command_prefix=command_prefix, extra_options=serve_options,
        console_output=True, wait_until_ready=True)
    server_b_1 = driver.Process(cluster, files_b,
        command_prefix=command_prefix, extra_options=serve_options,
        console_output=True, wait_until_ready=True)
    conn = r.connect(host=server_a_1.host, port=server_a_1.driver_port)

    print("Creating a table (%.2fs)" % (time.time() - startTime))
    num_shards = 16
    r.db_create("test").run(conn)
    r.db("rethinkdb").table("table_config").insert({
        "name": "test",
        "db": "test",
        "shards": [{"primary_replica": "a", "replicas": ["a"]}] * num_shards
        }).run(conn)
    r.table("test").wait().run(conn)

    print("Inserting %d documents (%.2fs)" % (opts["num_rows"], time.time() - startTime))
    docs_so_far = 0
    while docs_so_far < opts["num_rows"]:
        chunk = min(opts["num_rows"] - docs_so_far, 1000)
        res = r.table("test").insert(r.range(chunk).map({
            "value": r.row,
            "padding": "x" * 100
            }), durability="soft").run(conn)
        assert res["inserted"] == chunk
        docs_so_far += chunk
        print("Progress: %d/%d (%.2fs)" %
            (docs_so_far, opts["num_rows"], time.time() - startTime))

    print("Beginning replication to second server (%.2fs)" % (time.time() - startTime))
    r.table("test").config().update({
        "shards": [{"primary_replica": "a", "replicas": ["a", "b"]}] * num_shards
        }).run(conn)
    status = r.table("test").status().run(conn)
    assert status["status"]["ready_for_writes"], status
    assert not status["status"]["all_replicas_ready"], status

    print("Waiting a few seconds for backfill to get going (%.2fs)" % (time.time() - startTime))
    time.sleep(2)

    print("Shutting down both servers (%.2fs)" % (time.time() - startTime))
    server_a_1.check_and_stop()
    server_b_1.check_and_stop()

    print("Restarting both servers (%.2fs)" % (time.time() - startTime))
    server_a_2 = driver.Process(cluster, files_a,
        command_prefix=command_prefix, extra_options=serve_options,
        console_output=True, wait_until_ready=True)
    server_b_2 = driver.Process(cluster, files_b,
        command_prefix=command_prefix, extra_options=serve_options,
        console_output=True, wait_until_ready=True)
    conn_a = r.connect(host=server_a_2.host, port=server_a_2.driver_port)
    conn_b = r.connect(host=server_b_2.host, port=server_b_2.driver_port)

    print("Checking that table is available for writes (%.2fs)" % (time.time() - startTime))
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

    print("Making sure the backfill didn't end (%.2fs)" % (time.time() - startTime))
    status = r.table("test").status().run(conn_a)
    assert not status["status"]["all_replicas_ready"], status

    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))
