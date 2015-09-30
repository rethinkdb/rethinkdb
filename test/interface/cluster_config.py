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
    
    print("Setting AuthKey (%.2fs)" % (time.time() - startTime))

    assert list(r.db("rethinkdb").table("cluster_config").run(conn)) == [
            {"id": "auth", "auth_key": None},
            {"id": "heartbeat", "heartbeat_timeout_secs": 10}
        ]
    
    res = r.db("rethinkdb").table("cluster_config").get("auth").update({"auth_key": "hunter2"}).run(conn)
    assert res["errors"] == 0
    
    conn.close()
    
    print("Verifying AuthKey prevents unauthenticated connections (%.2fs)" % (time.time() - startTime))
    
    try:
        r.connect(server.host, server.driver_port)
    except r.ReqlDriverError:
        pass
    else:
        assert False, "the change to the auth key doesn't seem to have worked"
    
    print("Checking connection with the AuthKey (%.2fs)" % (time.time() - startTime))
    
    conn = r.connect(server.host, server.driver_port, auth_key="hunter2")
    
    rows = list(r.db("rethinkdb").table("cluster_config").filter({"id": "auth"}).run(conn))
    assert rows == [{"id": "auth", "auth_key": {"hidden": True}}]
    
    print("Removing the AuthKey (%.2fs)" % (time.time() - startTime))
    
    res = r.db("rethinkdb").table("cluster_config").get("auth").update({"auth_key": None}).run(conn)
    assert res["errors"] == 0
    
    assert list(r.db("rethinkdb").table("cluster_config").filter({"id": "auth"}).run(conn)) == [{"id": "auth", "auth_key": None}]
    
    conn.close()
    
    print("Verifying connection without an AuthKey (%.2fs)" % (time.time() - startTime))
    
    conn = r.connect(server.host, server.driver_port)
    assert list(r.db("rethinkdb").table("cluster_config").filter({"id": "auth"}).run(conn)) == [{"id": "auth", "auth_key": None}]

    # This is mostly to make sure the server doesn't crash in this case
    res = r.db("rethinkdb").table("cluster_config").get("auth").update({"auth_key": {"hidden": True}}).run(conn)
    assert res["errors"] == 1

    print("Inserting nonsense to make sure it's not accepted (%.2fs)" % (time.time() - startTime))
    
    res = r.db("rethinkdb").table("cluster_config").insert({"id": "foobar"}).run(conn)
    assert res["errors"] == 1, res
    res = r.db("rethinkdb").table("cluster_config").get("auth").delete().run(conn)
    assert res["errors"] == 1, res
    res = r.db("rethinkdb").table("cluster_config").get("auth") \
           .update({"foo": "bar"}).run(conn)
    assert res["errors"] == 1, res
    res = r.db("rethinkdb").table("cluster_config").get("auth") \
           .update({"auth_key": {"is_this_nonsense": "yes"}}).run(conn)
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

