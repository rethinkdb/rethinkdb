#!/usr/bin/env python
# Copyright 2014 RethinkDB, all rights reserved.

import sys, os, time, pprint

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
    files2 = driver.Files(metacluster, db_path="db-2", console_output="create-output-2", server_name="b", server_tags=["foo", "bar"], command_prefix=command_prefix)
    process1 = driver.Process(cluster, files1, console_output="serve-output-1", command_prefix=command_prefix, extra_options=serve_options + ["--cache-size", "auto"])
    process2 = driver.Process(cluster, files2, console_output="serve-output-2", command_prefix=command_prefix, extra_options=serve_options + ["--cache-size", "123"])
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

    print("Checking initial cache size...")
    res = r.db("rethinkdb").table("server_config") \
           .get(uuid_a)["cache_size_mb"].run(reql_conn1)
    assert res == "auto", res
    res = r.db("rethinkdb").table("server_config") \
           .get(uuid_b)["cache_size_mb"].run(reql_conn1)
    assert res == 123, res
    res = r.db("rethinkdb").table("server_status") \
           .get(uuid_b)["process"]["cache_size_mb"].run(reql_conn1)
    assert res == 123, res

    print("Checking that cache size can be changed...")
    res = r.db("rethinkdb").table("server_config") \
           .get(uuid_b).update({"cache_size_mb": 234}) \
           .run(reql_conn1)
    assert res["errors"] == 0, res
    res = r.db("rethinkdb").table("server_config") \
           .get(uuid_b)["cache_size_mb"].run(reql_conn1)
    assert res == 234
    res = r.db("rethinkdb").table("server_status") \
           .get(uuid_b)["process"]["cache_size_mb"].run(reql_conn1)
    assert res == 234, res

    print("Checking that absurd cache sizes are rejected...")
    def try_bad_cache_size(size, message):
        res = r.db("rethinkdb").table("server_config") \
               .get(uuid_b).update({"cache_size_mb": r.literal(size)}) \
               .run(reql_conn1)
        assert res["errors"] == 1, res
        assert message in res["first_error"]
    try_bad_cache_size("foobar", "wrong format")
    try_bad_cache_size(-30, "wrong format")
    try_bad_cache_size({}, "wrong format")
    # 2**40 is chosen so that it fits into a 64-bit integer when expressed in bytes, to
    # test the code path where the value is sent to the other server but then rejected by
    # validate_total_cache_size().
    try_bad_cache_size(2**40, "Error when trying to change the cache size of server")
    # 2**100 is chosen so that it doesn't fit into a 64-bit integer, so it will take a
    # different code path and get a different error message.
    try_bad_cache_size(2**100, "wrong format")

    cluster.check_and_stop()

