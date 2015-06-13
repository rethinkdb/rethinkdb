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
        # Wait for up to 5 seconds because changes to the cache size might take a moment
        # to take effect.
        deadline = time.time() + 5
        last_error = None
        while time.time() < deadline:
            try:
                actual_mb = r.db("rethinkdb") \
                             .table("stats") \
                             ["storage_engine"]["cache"]["in_use_bytes"] \
                             .nth(0) \
                             .div(megabyte) \
                             .run(conn)
                assert expect_mb * 0.9 <= actual_mb <= expect_mb * 1.01, "Measured cache usage at %.2f MB (expected %.2f MB)" % (actual_mb, expect_mb)
                break
            except AssertionError as e:
                last_error = e
                time.sleep(0.5)
        else:
            raise last_error

    high_cache_mb = 20
    set_cache_size(high_cache_mb)

    print("Making a table. (%.2fs)" % (time.time() - startTime))
    r.db_create("test").run(conn)
    r.table_create("test").run(conn)

    print("Filling table (%.2fs)" % (time.time() - startTime))
    doc_size = 10000
    overfill_factor = 1.5
    num_docs = int(high_cache_mb * megabyte * overfill_factor / doc_size)
    res = r.table("test").insert(r.range(num_docs).map(
        lambda x: {"id":x, "payload":"a" * doc_size}
    ), durability="soft").run(conn)
    assert res["inserted"] == num_docs and res["errors"] == 0, res

    # We might have to run a read or two to make sure the cache is actually scaled
    # up to its configured maximum size. This is because cache allocation to a table
    # might lack behind a little.
    last_error = None
    for i in range(0, 2):
        try:
            res = r.table("test").map(r.row).count().run(conn)
            assert res == num_docs, res

            check_cache_usage(high_cache_mb)
            break
        except AssertionError as e:
            last_error = e
    else:
        raise last_error

    low_cache_mb = 10
    set_cache_size(low_cache_mb)

    check_cache_usage(low_cache_mb)

    set_cache_size(high_cache_mb)

    print("Forcing cache repopulation...")
    # Try this twice in case the new cache size hasn't taken effect yet the first time
    # we run the query.
    last_error = None
    for i in range(0, 2):
        try:
            res = r.table("test").map(r.row).count().run(conn)
            assert res == num_docs, res

            check_cache_usage(high_cache_mb)
            break
        except AssertionError as e:
            last_error = e
    else:
        raise last_error

    set_cache_size(0)
    check_cache_usage(0)

    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))
