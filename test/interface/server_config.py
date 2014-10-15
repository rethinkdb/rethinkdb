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
        server_name = "a", server_tags = ["foo"],
        executable_path = executable_path, command_prefix = command_prefix)
    files2 = driver.Files(
        metacluster, db_path = "db-2", log_path = "create_output-2",
        server_name = "b", server_tags = ["foo", "bar"],
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

    assert r.db("rethinkdb").table("server_config").count().run(reql_conn1) == 2
    uuid_a = r.db("rethinkdb").table("server_config").filter({"name":"a"}) \
              .nth(0)["uuid"].run(reql_conn1)
    uuid_b = r.db("rethinkdb").table("server_config").filter({"name":"b"}) \
              .nth(0)["uuid"].run(reql_conn1)

    def check_name(uuid, expect_name):
        timeout = 10
        for i in xrange(timeout):
            names = [r.db("rethinkdb").table("server_config").get(uuid)["name"].run(c)
                     for c in [reql_conn1, reql_conn2]]
            if names[0] == names[1] == expect_name:
                break
            else:
                time.sleep(1)
        else:
            raise RuntimeError("Name isn't as expected even after %d seconds" % timeout)

    print "Checking initial names..."
    check_name(uuid_a, "a")
    check_name(uuid_b, "b")
    cluster.check()

    print "Checking changing name locally..."
    res = r.db("rethinkdb").table("server_config").get(uuid_a).update({"name": "a2"}) \
           .run(reql_conn1)
    assert res["errors"] == 0
    check_name(uuid_a, "a2")
    check_name(uuid_b, "b")
    cluster.check()

    print "Checking changing name remotely..."
    res = r.db("rethinkdb").table("server_config").get(uuid_b).update({"name": "b2"}) \
           .run(reql_conn1)
    assert res["errors"] == 0
    check_name(uuid_a, "a2")
    check_name(uuid_b, "b2")
    cluster.check()

    print "Checking that name conflicts are rejected..."
    res = r.db("rethinkdb").table("server_config").get(uuid_a).update({"name": "b2"}) \
           .run(reql_conn1)
    assert res["errors"] == 1
    assert "already exists" in res["first_error"]
    check_name(uuid_a, "a2")
    check_name(uuid_b, "b2")
    cluster.check()

    def check_tags(uuid, expect_tags):
        timeout = 10
        for i in xrange(timeout):
            tags = [r.db("rethinkdb").table("server_config").get(uuid)["tags"].run(c)
                    for c in [reql_conn1, reql_conn2]]
            if set(tags[0]) == set(tags[1]) == set(expect_tags):
                break
            else:
                time.sleep(1)
        else:
            raise RuntimeError("Tags aren't as expected even after %d seconds" % timeout)

    print "Checking initial tags..."
    check_tags(uuid_a, ["default", "foo"])
    check_tags(uuid_b, ["default", "foo", "bar"])
    cluster.check()

    print "Checking changing tags locally..."
    res = r.db("rethinkdb").table("server_config") \
           .get(uuid_a).update({"tags": ["baz"]}) \
           .run(reql_conn1)
    assert res["errors"] == 0
    check_tags(uuid_a, ["baz"])
    check_tags(uuid_b, ["default", "foo", "bar"])
    cluster.check()

    print "Checking changing tags remotely..."
    res = r.db("rethinkdb").table("server_config") \
           .get(uuid_b).update({"tags": ["quz"]}) \
           .run(reql_conn1)
    assert res["errors"] == 0
    check_tags(uuid_a, ["baz"])
    check_tags(uuid_b, ["quz"])
    cluster.check()

    print "Checking that invalid tags are rejected..."
    res = r.db("rethinkdb").table("server_config") \
           .get(uuid_a).update({"tags": [":-)"]}) \
           .run(reql_conn1)
    assert res["errors"] == 1, "It shouldn't be possible to set tags that aren't " \
        "valid names."
    check_tags(uuid_a, ["baz"])
    check_tags(uuid_b, ["quz"])
    cluster.check()

    cluster.check_and_stop()

