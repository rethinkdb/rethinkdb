#!/usr/bin/env python
# Copyright 2014 RethinkDB, all rights reserved.

import sys, os, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

r = utils.import_python_driver()

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    _, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    
    print("Spinning up two processes...")
    files1 = driver.Files(metacluster, db_path="db-1", console_output="create-output-1", server_name="a", server_tags=["foo"], command_prefix=command_prefix)
    files2 = driver.Files(metacluster, db_path="db-2", console_output="create-output-2", server_name="a", server_tags=["foo"], command_prefix=command_prefix)
    process1 = driver.Process(cluster, files1, console_output="serve-output-1", command_prefix=command_prefix, extra_options=serve_options)
    process2 = driver.Process(cluster, files2, console_output="serve-output-2", command_prefix=command_prefix, extra_options=serve_options)
    process1.wait_until_started_up()
    process2.wait_until_started_up()
    cluster.check()
    
    reql_conn1 = r.connect(process1.host, process1.driver_port)
    reql_conn2 = r.connect(process2.host, process2.driver_port)

    assert r.db("rethinkdb").table("server_config").count().run(reql_conn1) == 2
    uuid_a = r.db("rethinkdb").table("server_config").filter({"name":"a"}) \
              .nth(0)["id"].run(reql_conn1)
    uuid_b = r.db("rethinkdb").table("server_config").filter({"name":"b"}) \
              .nth(0)["id"].run(reql_conn1)

    def check_name(uuid, expect_name):
        names = [r.db("rethinkdb").table("server_config").get(uuid)["name"].run(c)
                 for c in [reql_conn1, reql_conn2]]
        assert names[0] == names[1] == expect_name

    print("Checking initial names...")
    check_name(uuid_a, "a")
    check_name(uuid_b, "b")
    cluster.check()

    print("Checking changing name locally...")
    res = r.db("rethinkdb").table("server_config").get(uuid_a).update({"name": "a2"}) \
           .run(reql_conn1)
    assert res["errors"] == 0
    check_name(uuid_a, "a2")
    check_name(uuid_b, "b")
    cluster.check()

    print("Checking changing name remotely...")
    res = r.db("rethinkdb").table("server_config").get(uuid_b).update({"name": "b2"}) \
           .run(reql_conn1)
    assert res["errors"] == 0
    check_name(uuid_a, "a2")
    check_name(uuid_b, "b2")
    cluster.check()

    print("Checking that name conflicts are rejected...")
    res = r.db("rethinkdb").table("server_config").get(uuid_a).update({"name": "b2"}) \
           .run(reql_conn1)
    assert res["errors"] == 1
    assert "already exists" in res["first_error"]
    check_name(uuid_a, "a2")
    check_name(uuid_b, "b2")
    cluster.check()

    def check_tags(uuid, expect_tags):
        tags = [r.db("rethinkdb").table("server_config").get(uuid)["tags"].run(c)
                for c in [reql_conn1, reql_conn2]]
        assert set(tags[0]) == set(tags[1]) == set(expect_tags)

    print("Checking initial tags...")
    check_tags(uuid_a, ["default", "foo"])
    check_tags(uuid_b, ["default", "foo", "bar"])
    cluster.check()

    print("Checking changing tags locally...")
    res = r.db("rethinkdb").table("server_config") \
           .get(uuid_a).update({"tags": ["baz"]}) \
           .run(reql_conn1)
    assert res["errors"] == 0
    check_tags(uuid_a, ["baz"])
    check_tags(uuid_b, ["default", "foo", "bar"])
    cluster.check()

    print("Checking changing tags remotely...")
    res = r.db("rethinkdb").table("server_config") \
           .get(uuid_b).update({"tags": ["quz"]}) \
           .run(reql_conn1)
    assert res["errors"] == 0
    check_tags(uuid_a, ["baz"])
    check_tags(uuid_b, ["quz"])
    cluster.check()

    print("Checking that invalid tags are rejected...")
    res = r.db("rethinkdb").table("server_config") \
           .get(uuid_a).update({"tags": [":-)"]}) \
           .run(reql_conn1)
    assert res["errors"] == 1, "It shouldn't be possible to set tags that aren't " \
        "valid names."
    check_tags(uuid_a, ["baz"])
    check_tags(uuid_b, ["quz"])
    cluster.check()

    cluster.check_and_stop()

