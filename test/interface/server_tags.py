#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(
    os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils
r = utils.import_python_driver()
from vcoptparse import *

op = OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = \
        scenario_common.parse_mode_flags(opts)
    print "Spinning up two processes..."
    files1 = driver.Files(
        metacluster, db_path = "db-1", log_path = "create-output-1",
        machine_name = "a", server_tags = ["foo"],
        executable_path = executable_path, command_prefix = command_prefix)
    files2 = driver.Files(
        metacluster, db_path = "db-2", log_path = "create_output-2",
        machine_name = "b", server_tags = ["foo", "bar"],
        executable_path = executable_path, command_prefix = command_prefix)
    process1 = driver.Process(cluster, files1, log_path="serve-output-1",
        executable_path = executable_path, command_prefix = command_prefix,
        extra_options = serve_options)
    time.sleep(3)
    process2 = driver.Process(cluster, files2, log_path="serve-output-2",
        executable_path = executable_path, command_prefix = command_prefix,
        extra_options = serve_options)
    process1.wait_until_started_up()
    process2.wait_until_started_up()
    cluster.check()
    reql_conn1 = r.connect("localhost", process1.driver_port)
    reql_conn2 = r.connect("localhost", process2.driver_port)

    def check_tags(server_name, expect_tags):
        tags = [r.db("rethinkdb").table("server_config") \
                 .get(server_name)["tags"] \
                 .run(c) for c in [reql_conn1, reql_conn2]]
        assert set(tags[0]) == set(expect_tags)
        assert set(tags[1]) == set(expect_tags)

    print "Checking initial tags..."
    check_tags("a", ["default", "foo"])
    check_tags("b", ["default", "foo", "bar"])
    cluster.check()

    print "Checking changing tags locally..."
    r.db("rethinkdb").table("server_config") \
     .get("a").update({"tags": ["baz"]}) \
     .run(reql_conn1)
    check_tags("a", ["baz"])
    check_tags("b", ["default", "foo", "bar"])
    cluster.check()

    print "Checking changing tags remotely..."
    r.db("rethinkdb").table("server_config") \
     .get("b").update({"tags": ["quz"]}) \
     .run(reql_conn1)
    check_tags("a", ["baz"])
    check_tags("b", ["quz"])
    cluster.check()

    print "Checking that invalid tags are rejected..."
    try:
        r.db("rethinkdb").table("server_config") \
         .get("a").update({"tags": [":)"]}) \
         .run(reql_conn1)
    except r.RqlRuntimeError:
        pass
    else:
        assert False, "It shouldn't be possible to set tags that aren't valid names."
    check_tags("a", ["baz"])
    check_tags("b", ["quz"])
    cluster.check()

    cluster.check_and_stop()

