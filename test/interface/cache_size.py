#!/usr/bin/env python
# Copyright 2014 RethinkDB, all rights reserved.
from __future__ import print_function
import os, subprocess, sys, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse
r = utils.import_python_driver()

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    _, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    
    print("Spinning up a process...")
    files = driver.Files(metacluster, db_path="db", console_output="create-output", command_prefix=command_prefix)
    proc = driver.Process(cluster, files, console_output="serve-output", command_prefix=command_prefix, extra_options=serve_options)
    proc.wait_until_started_up()
    cluster.check()
    conn = r.connect(proc.host, proc.driver_port)

    def set_cache_size(size_mb):
        print("Setting cache size to %.2f MB." % size_mb)
        res = r.db("rethinkdb").table("server_config") \
               .update({"cache_size_mb": size_mb}) \
               .run(conn)
        assert res["errors"] == 0, res

    def check_cache_usage(expect_mb):
        actual_mb = r.db("rethinkdb") \
                     .table("stats") \
                     ["storage_engine"]["cache"]["in_use_bytes"] \
                     .nth(0) \
                     .div(1e6) \
                     .run(conn)
        print("Measured cache usage at %.2f MB (expected %.2f MB)" %
            (actual_mb, expect_mb))
        assert expect_mb * 0.8 <= actual_mb <= expect_mb * 1.2

    high_cache_mb = 10
    set_cache_size(high_cache_mb)

    print("Making a table.")
    # Create a table and fill the cache with data
    r.db_create("test").run(conn)
    r.table_create("test").run(conn)

    print("Filling table...")
    doc_size = 10000
    num_docs = int(high_cache_mb * 1e6 / doc_size)
    res = \
        r.range(num_docs).for_each(
            r.table("test").insert(
                {"id": r.row, "payload": "a"*doc_size})) \
        .run(conn)
    assert res["inserted"] == num_docs and res["errors"] == 0, res

    check_cache_usage(high_cache_mb)

    low_cache_mb = 5
    set_cache_size(low_cache_mb)

    check_cache_usage(low_cache_mb)

    set_cache_size(high_cache_mb)

    print("Forcing cache repopulation...")
    res = r.table("test").update({"new_field": "hi"}).run(conn)
    assert res["replaced"] == num_docs and res["errors"] == 0, res

    check_cache_usage(high_cache_mb)

    cluster.check_and_stop()

