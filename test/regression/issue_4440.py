#!/usr/bin/env python
# Copyright 2014 RethinkDB, all rights reserved.

from __future__ import print_function

import os, pprint, sys, time, traceback

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

r = utils.import_python_driver()

print("Starting cluster of two servers (%.2fs)" % (time.time() - startTime))
with driver.Cluster(
        initial_servers = ["a", "b"],
        output_folder = '.',
        command_prefix = command_prefix,
        extra_options = serve_options,
        wait_until_ready = True) as cluster:

    print("Establishing ReQl connections (%.2fs)" % (time.time() - startTime))
    conn = r.connect(host=cluster[0].host, port=cluster[0].driver_port)

    print("Creating a table (%.2fs)" % (time.time() - startTime))
    r.db_create("test").run(conn)
    res = r.table_create("test", replicas=2).run(conn)
    table_uuid = res["config_changes"][0]["new_val"]["id"]

    r.table("test").insert(r.range(1000).map({"value": r.row})).run(conn)

    assert os.path.exists(os.path.join(cluster[0].files.db_path, table_uuid))
    assert os.path.exists(os.path.join(cluster[1].files.db_path, table_uuid))

    print("Removing one replica (%.2fs)" % (time.time() - startTime))
    r.table("test").config().update(
        {"shards": [{"primary_replica": "a", "replicas": ["a"]}]}
        ).run(conn)
    r.table("test").wait(wait_for="all_replicas_ready").run(conn)

    # This timer actually needs to be this long; with a 5-second timer I was still seeing
    # intermittent failures.
    nap_length = 10
    print("Waiting %d seconds (%.2fs)" % (nap_length, time.time() - startTime))
    time.sleep(nap_length)

    print("Confirming files were deleted (%.2fs)" % (time.time() - startTime))
    assert os.path.exists(os.path.join(cluster[0].files.db_path, table_uuid))
    assert not os.path.exists(os.path.join(cluster[1].files.db_path, table_uuid))

    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))
