#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.
import sys, os, time, traceback
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils
from vcoptparse import *
r = utils.import_python_driver()

"""The `interface.cluster_config` test checks that the special `rethinkdb.cluster_config`
table behaves as expected."""

op = OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    print "Spinning up a process..."
    files = driver.Files(metacluster, log_path = "create-output", machine_name = "a",
                         executable_path = executable_path, command_prefix = command_prefix)
    proc = driver.Process(cluster, files, log_path = "serve-output",
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
    proc.wait_until_started_up()
    cluster.check()
    conn = r.connect("localhost", proc.driver_port)

    rows = list(r.db("rethinkdb").table("cluster_config").run(conn))
    assert rows == [{"id": "auth", "auth_key": None}]

    res = r.db("rethinkdb").table("cluster_config").get("auth") \
           .update({"auth_key": "hunter2"}).run(conn)
    assert res["errors"] == 0

    rows = list(r.db("rethinkdb").table("cluster_config").run(conn))
    assert rows == [{"id": "auth", "auth_key": {"hidden": True}}]

    try:
        r.connect("localhost", proc.driver_port)
    except r.RqlDriverError:
        pass
    else:
        raise ValueError("the change to the auth key doesn't seem to have worked")

    r.connect("localhost", proc.driver_port, auth_key="hunter2").close()

    res = r.db("rethinkdb").table("cluster_config").get("auth") \
           .update({"auth_key": None}).run(conn)
    assert res["errors"] == 0

    rows = list(r.db("rethinkdb").table("cluster_config").run(conn))
    assert rows == [{"id": "auth", "auth_key": None}]

    r.connect("localhost", proc.driver_port).close()

    # This is mostly to make sure the server doesn't crash in this case
    res = r.db("rethinkdb").table("cluster_config").get("auth") \
           .update({"auth_key": {"hidden": True}}).run(conn)
    assert res["errors"] == 1

    cluster.check_and_stop()
print "Done."

