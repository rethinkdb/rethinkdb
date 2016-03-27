#!/usr/bin/env python
# Copyright 2015-2016 RethinkDB, all rights reserved.

import os, sys, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, server_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

r = utils.import_python_driver()

with driver.Cluster(initial_servers=["a", "b"], output_folder='.', command_prefix=command_prefix, extra_options=server_options) as cluster:

    utils.print_with_time("Establishing ReQl connections")
    conn = r.connect(host=cluster[0].host, port=cluster[0].driver_port)

    utils.print_with_time("Creating a table")
    r.db_create("test").run(conn)
    res = r.table_create("test", replicas=2).run(conn)
    table_uuid = res["config_changes"][0]["new_val"]["id"]
    
    r.table("test").wait().run(conn)
    r.table("test").insert(r.range(1000).map({"value": r.row})).run(conn)
    
    assert os.path.exists(os.path.join(cluster[0].data_path, table_uuid))
    assert os.path.exists(os.path.join(cluster[1].data_path, table_uuid))

    utils.print_with_time("Removing one replica")
    r.table("test").config().update({"shards": [{"primary_replica": "a", "replicas": ["a"]}]}).run(conn)
    r.table("test").wait(wait_for="all_replicas_ready").run(conn)

    # This timer actually needs to be this long; with a 5-second timer I was still seeing intermittent failures.
    nap_length = 10
    utils.print_with_time("Waiting %d seconds" % nap_length)
    time.sleep(nap_length)

    utils.print_with_time("Confirming files were deleted")
    assert os.path.exists(os.path.join(cluster[0].data_path, table_uuid))
    assert not os.path.exists(os.path.join(cluster[1].data_path, table_uuid))

    utils.print_with_time("Cleaning up")
utils.print_with_time("Done.")
