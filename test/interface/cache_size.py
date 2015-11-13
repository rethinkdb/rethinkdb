#!/usr/bin/env python
# Copyright 2014-2015 RethinkDB, all rights reserved.

import os, sys, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()

megabyte = 1024 * 1024

utils.print_with_time("Starting a server")
with driver.Process(console_output=True, name='.', command_prefix=command_prefix, extra_options=serve_options, wait_until_ready=True) as server:
    
    utils.print_with_time("Establishing ReQL connection")
    conn = r.connect(host=server.host, port=server.driver_port)

    def set_cache_size(size_mb):
        utils.print_with_time("Setting cache size to %.2f MB." % size_mb)
        res = r.db("rethinkdb").table("server_config").update({"cache_size_mb": size_mb}).run(conn)
        assert res["errors"] == 0, res

    def check_cache_usage(expect_mb):
        # Wait for up to 5 seconds or 2 retries (whichever comes later) because changes
        # to the cache size might take a moment to take effect.
        deadline = time.time() + 5
        num_tries = 0
        last_error = None
        while time.time() < deadline or num_tries < 2:
            num_tries = num_tries + 1
            try:
                # Read from the table to make sure that the cache gets filled up.
                res = r.db(dbName).table(tableName).map(r.row).count().run(conn)
                assert res == num_docs, res

                actual_mb = r.db("rethinkdb").table("stats")["storage_engine"]["cache"]["in_use_bytes"].nth(0).div(megabyte).run(conn)
                assert expect_mb * 0.9 <= actual_mb <= expect_mb * 1.01, "Measured cache usage at %.2f MB (expected %.2f MB)" % (actual_mb, expect_mb)
                break
            except AssertionError as e:
                last_error = e
                time.sleep(0.5)
        else:
            raise last_error

    high_cache_mb = 20
    set_cache_size(high_cache_mb)

    utils.print_with_time("Making a table.")
    r.expr([dbName]).set_difference(r.db_list()).for_each(r.db_create(r.row)).run(conn)
    r.db(dbName).table_create(tableName).run(conn)

    utils.print_with_time("Filling table")
    doc_size = 10000
    overfill_factor = 1.5
    num_docs = int(high_cache_mb * megabyte * overfill_factor / doc_size)
    res = r.db(dbName).table(tableName).insert(r.range(num_docs).map(lambda x: {"id":x, "payload":"a" * doc_size}), durability="soft").run(conn)
    assert res["inserted"] == num_docs and res["errors"] == 0, res

    check_cache_usage(high_cache_mb)

    low_cache_mb = 10
    set_cache_size(low_cache_mb)
    check_cache_usage(low_cache_mb)

    set_cache_size(high_cache_mb)
    check_cache_usage(high_cache_mb)

    set_cache_size(0)
    check_cache_usage(0)

    utils.print_with_time("Cleaning up")
utils.print_with_time("Done.")
