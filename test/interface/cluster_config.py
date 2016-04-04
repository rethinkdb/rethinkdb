#!/usr/bin/env python
# Copyright 2014-2015 RethinkDB, all rights reserved.

"""The `interface.cluster_config` test checks that the special `rethinkdb.cluster_config` table behaves as expected."""

import os, sys, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

r = utils.import_python_driver()

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

print("Starting a server (%.2fs)" % (time.time() - startTime))
with driver.Cluster(initial_servers=1, output_folder='.', wait_until_ready=True, command_prefix=command_prefix, extra_options=serve_options) as cluster:
    
    print("Establishing ReQL connection (%.2fs)" % (time.time() - startTime))
    server = cluster[0]
    conn = r.connect(server.host, server.driver_port)

    print("Inserting nonsense to make sure it's not accepted (%.2fs)" % (time.time() - startTime))
    
    res = r.db("rethinkdb").table("cluster_config").insert({"id": "foobar"}).run(conn)
    assert res["errors"] == 1, res
    res = r.db("rethinkdb").table("cluster_config").get("heartbeat").delete().run(conn)
    assert res["errors"] == 1, res
    res = r.db("rethinkdb").table("cluster_config").get("heartbeat") \
           .update({"foo": "bar"}).run(conn)
    assert res["errors"] == 1, res

    print("Verifying heartbeat timeout interface")

    res = r.db("rethinkdb").table("cluster_config").get("heartbeat").update({"heartbeat_timeout_secs": -1.0}).run(conn)
    assert res["errors"] == 1, res
    res = r.db("rethinkdb").table("cluster_config").get("heartbeat").update({"heartbeat_timeout_secs": 1.0}).run(conn)
    assert res["errors"] == 1, res
    res = r.db("rethinkdb").table("cluster_config").get("heartbeat").update({"heartbeat_timeout_secs": 2.0}).run(conn)
    assert res["replaced"] == 1, res
    res = r.db("rethinkdb").table("cluster_config").get("heartbeat").run(conn)
    assert res["heartbeat_timeout_secs"] == 2
    res = r.db("rethinkdb").table("cluster_config").get("heartbeat").update({"heartbeat_timeout_secs": 2.5}).run(conn)
    assert res["replaced"] == 1, res
    res = r.db("rethinkdb").table("cluster_config").get("heartbeat").run(conn)
    assert res["heartbeat_timeout_secs"] == 2.5

    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))

