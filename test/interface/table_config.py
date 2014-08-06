#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.
import sys, os, time, traceback
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils
from vcoptparse import *
r = utils.import_pyton_driver()

op = OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster1 = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    print "Spinning up two processes..."
    files1 = driver.Files(metacluster, log_path = "create-output-1", machine_name = "a",
                          executable_path = executable_path, command_prefix = command_prefix)
    proc1 = driver.Process(cluster1, files1, log_path = "serve-output-1",
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
    files2 = driver.Files(metacluster, log_path = "create-output-2", machine_name = "b",
                          executable_path = executable_path, command_prefix = command_prefix)
    proc2 = driver.Process(cluster1, files2, log_path = "serve-output-2",
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
    proc1.wait_until_started_up()
    proc2.wait_until_started_up()
    cluster1.check()

    print "Creating a table..."
    conn = r.connect("localhost", proc1.driver_port)
    r.db_create("test").run(conn)
    r.table_create("foo").run(conn)
    r.table("foo").insert([{"i": i} for i in xrange(10)]).run(conn)
    assert set(row["i"] for row in r.table("foo").run(conn)) == set(xrange(10))

    print "Reconfiguring table: director 'a'"
    r.db("rethinkdb").table("table_config").get("foo").update(
        {"shards": r.literal([{"replicas": ["a"], "directors": ["a"]}])}).run(conn)
    print "Cluster should now reconfigure."
    start_time = time.time()
    while time.time() < start_time + 10:
        status = r.db("rethinkdb").table("table_status").get("foo").run(conn)
        assert status["name"] == "foo"
        shard = status["shards"][0]
        if shard["a"]["state"] == "ready" and shard["a"]["role"] == "director" and \
                "b" not in status["shards"][0]:
            break
    else:
        raise RuntimeError("Cluster didn't reconfigure; status=%r" % status)
    assert set(row["i"] for row in r.table("foo").run(conn)) == set(xrange(10))

    print "Reconfiguring table: director 'b'"
    r.db("rethinkdb").table("table_config").get("foo").update(
        {"shards": [{"replicas": ["b"], "directors": ["b"]}]}).run(conn)
    print "Cluster should now reconfigure."
    start_time = time.time()
    while time.time() < start_time + 10:
        status = r.db("rethinkdb").table("table_status").get("foo").run(conn)
        assert status["name"] == "foo"
        shard = status["shards"][0]
        if shard["b"]["state"] == "ready" and shard["b"]["role"] == "director" and \
                "a" not in status["shards"][0]:
            break
    else:
        raise RuntimeError("Cluster didn't reconfigure; status=%r" % status)
    assert set(row["i"] for row in r.table("foo").run(conn)) == set(xrange(10))

    print "Reconfiguring table: director 'a', replica 'b'"
    r.db("rethinkdb").table("table_config").get("foo").update(
        {"shards": [{"replicas": ["a", "b"], "directors": ["a"]}]}).run(conn)
    print "Cluster should now reconfigure."
    start_time = time.time()
    while time.time() < start_time + 10:
        status = r.db("rethinkdb").table("table_status").get("foo").run(conn)
        assert status["name"] == "foo"
        shard = status["shards"][0]
        if shard["a"]["state"] == "ready" and shard["a"]["role"] == "director" and \
                shard["b"]["state"] == "ready" and shard["b"]["role"] == "replica":
            break
    else:
        raise RuntimeError("Cluster didn't reconfigure; status=%r" % status)
    assert set(row["i"] for row in r.table("foo").run(conn)) == set(xrange(10))

    cluster1.check_and_stop()
print "Done."

