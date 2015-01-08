#!/usr/bin/env python
# Copyright 2014 RethinkDB, all rights reserved.

from __future__ import print_function

import os, sys, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

r = utils.import_python_driver()

megabyte = 1024 * 1024

print("Starting a server (%.2fs)" % (time.time() - startTime))
with driver.Process(console_output=True, output_folder='.', command_prefix=command_prefix, extra_options=serve_options, wait_until_ready=True) as server:
    
    print("Establishing ReQL connection (%.2fs)" % (time.time() - startTime))
    conn = r.connect(host=server.host, port=server.driver_port)


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
                     .div(megabyte) \
                     .run(conn)
        assert expect_mb * 0.9 <= actual_mb <= expect_mb * 1.01, "Measured cache usage at %.2f MB (expected %.2f MB)" % (actual_mb, expect_mb)

    high_cache_mb = 20
    set_cache_size(high_cache_mb)

    print("Making a table. (%.2fs)" % (time.time() - startTime))
    r.db_create("test").run(conn)
    r.table_create("test").run(conn)

    print("Filling table (%.2fs)" % (time.time() - startTime))
    doc_size = 10000
    overfill_factor = 1.2
    num_docs = int(high_cache_mb * megabyte * overfill_factor / doc_size)
    res = \
        r.range(num_docs).for_each(
            r.table("test").insert(
                {"id": r.row, "payload": "a" * doc_size})) \
        .run(conn)
    assert res["inserted"] == num_docs and res["errors"] == 0, res

    check_cache_usage(high_cache_mb)

    low_cache_mb = 10
    set_cache_size(low_cache_mb)

    check_cache_usage(low_cache_mb)

    set_cache_size(high_cache_mb)

    print("Forcing cache repopulation...")
    res = r.table("test").update({"new_field": "hi"}).run(conn)
    assert res["replaced"] == num_docs and res["errors"] == 0, res

    check_cache_usage(high_cache_mb)

    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))
