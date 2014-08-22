#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.
import sys, os, time, traceback
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils
from vcoptparse import *
r = utils.import_python_driver()

"""The `interface.table_config` test checks that the special `rethinkdb.table_config` and
`rethinkdb.table_status` tables behave as expected."""

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

    def status_matches_config(s_shards, c_shards):
        if len(s_shards) != len(c_shards):
            return False
        for (s_shard, c_shard) in zip(s_shards, c_shards):
            if set(s_shard.keys()) != set(c_shard["replicas"]):
                return False
            s_directors = [server for (server, server_status) in s_shard.iteritems()
                           if server_status["role"] == "director"]
            if len(s_directors) != 1:
                return False
            if s_directors[0] not in c_shard["directors"]:
                return False
            if any(server_status["state"] != "ready"
                   for server_status in s_shard.values()):
                return False
        return True

    def test(shards):
        print "Reconfiguring:", shards
        r.db("rethinkdb").table("table_config").get("test.foo").update(
            {"shards": shards}).run(conn)
        start_time = time.time()
        while time.time() < start_time + 10:
            status = r.db("rethinkdb").table("table_status").get("test.foo").run(conn)
            assert status["name"] == "test.foo"
            if status_matches_config(status["shards"], shards):
                break
            time.sleep(1)
        else:
            raise RuntimeError("Cluster didn't reconfigure; status=%r" % status)
        assert set(row["i"] for row in r.table("foo").run(conn)) == set(xrange(10))
        print "OK"

    test(
        [{"replicas": ["a"], "directors": ["a"]}])
    test(
        [{"replicas": ["b"], "directors": ["b"]}])
    test(
        [{"replicas": ["a", "b"], "directors": ["a"]}])
    test(
        [{"replicas": ["a"], "directors": ["a"]},
         {"replicas": ["b"], "directors": ["b"]}])
    test(
        [{"replicas": ["a", "b"], "directors": ["a"]},
         {"replicas": ["a", "b"], "directors": ["b"]}])
    test(
        [{"replicas": ["a"], "directors": ["a"]}])

    cluster1.check_and_stop()
print "Done."

