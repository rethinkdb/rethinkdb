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
    conn = r.connect("localhost", proc1.driver_port)

    def check_foo_config_matches(expected):
        config = r.table_config("foo").run(conn)
        assert config["name"] == "foo" and config["db"] == "test"
        found = config["shards"]
        if len(expected) != len(found):
            return False
        for (e_shard, f_shard) in zip(expected, found):
            if set(e_shard["replicas"]) != set(f_shard["replicas"]):
                return False
            if e_shard["director"] != f_shard["director"]:
                return False
        return True

    def check_status_matches_config():
        config = list(r.db("rethinkdb").table("table_config").run(conn))
        status = list(r.db("rethinkdb").table("table_status").run(conn))
        uuids = set(row["uuid"] for row in config)
        if not (len(uuids) == len(config) == len(status)):
            return False
        if uuids != set(row["uuid"] for row in status):
            return False
        for c_row in config:
            s_row = [row for row in status if row["uuid"] == c_row["uuid"]][0]
            if c_row["db"] != s_row["db"]:
                return False
            if c_row["name"] != s_row["name"]:
                return False
            c_shards = c_row["shards"]
            s_shards = s_row["shards"]
            if len(s_shards) != len(c_shards):
                return False
            for (s_shard, c_shard) in zip(s_shards, c_shards):
                if set(doc["server"] for doc in s_shard) != set(c_shard["replicas"]):
                    return False
                s_directors = [doc["server"] for doc in s_shard
                               if doc["role"] == "director"]
                if len(s_directors) != 1:
                    return False
                if s_directors[0] != c_shard["director"]:
                    return False
                if any(doc["state"] != "ready" for doc in s_shard):
                    return False
            if not s_row["ready_for_outdated_reads"]:
                return False
            if not s_row["ready_for_reads"]:
                return False
            if not s_row["ready_for_writes"]:
                return False
            if not s_row["ready_completely"]:
                return False
        return True

    def check_tables_named(names):
        config = list(r.db("rethinkdb").table("table_config").run(conn))
        if len(config) != len(names):
            return False
        for row in config:
            if (row["db"], row["name"]) not in names:
                return False
        return True

    def wait_until(condition):
        try:
            start_time = time.time()
            while not condition():
                time.sleep(1)
                if time.time() > start_time + 10:
                    raise RuntimeError("Out of time")
        except:
            config = list(r.db("rethinkdb").table("table_config").run(conn))
            status = list(r.db("rethinkdb").table("table_status").run(conn))
            print "Something went wrong."
            print "config =", config
            print "status =", status
            raise

    print "Creating a table..."
    r.db_create("test").run(conn)
    r.table_create("foo").run(conn)
    r.table_create("bar").run(conn)
    r.db_create("test2").run(conn)
    r.db("test2").table_create("bar2").run(conn)
    r.table("foo").insert([{"i": i} for i in xrange(10)]).run(conn)
    assert set(row["i"] for row in r.table("foo").run(conn)) == set(xrange(10))

    print "Testing that table_config and table_status are sane..."
    wait_until(lambda: check_tables_named(
        [("test", "foo"), ("test", "bar"), ("test2", "bar2")]))
    wait_until(check_status_matches_config)

    print "Testing that we can write to table_config..."
    def test(shards):
        print "Reconfiguring:", shards
        res = r.table_config("foo").update({"shards": shards}).run(conn)
        assert res["errors"] == 0, repr(res)
        wait_until(lambda: check_foo_config_matches(shards))
        wait_until(check_status_matches_config)
        assert set(row["i"] for row in r.table("foo").run(conn)) == set(xrange(10))
        print "OK"
    test(
        [{"replicas": ["a"], "director": "a"}])
    test(
        [{"replicas": ["b"], "director": "b"}])
    test(
        [{"replicas": ["a", "b"], "director": "a"}])
    test(
        [{"replicas": ["a"], "director": "a"},
         {"replicas": ["b"], "director": "b"}])
    test(
        [{"replicas": ["a", "b"], "director": "a"},
         {"replicas": ["a", "b"], "director": "b"}])
    test(
        [{"replicas": ["a"], "director": "a"}])

    print "Testing that table_config rejects invalid input..."
    def test_invalid(shards):
        print "Reconfiguring:", shards
        res = r.db("rethinkdb").table("table_config").filter({"name": "foo"}).update(
                {"shards": shards}).run(conn)
        assert res["errors"] == 1
        print "Error, as expected"
    test_invalid([])
    test_invalid("this is a string")
    test_invalid(
        [{"replicas": ["a"], "director": "a", "extra_key": "extra_value"}])
    test_invalid(
        [{"replicas": [], "director": None}])
    test_invalid(
        [{"replicas": ["a"], "director": "b"}])
    test_invalid(
        [{"replicas": ["a"], "director": "b"},
         {"replicas": ["b"], "director": "a"}])

    print "Testing that we can rename tables through table_config..."
    res = r.table_config("bar").update({"name": "bar2"}).run(conn)
    assert res["errors"] == 0
    wait_until(lambda: check_tables_named(
        [("test", "foo"), ("test", "bar2"), ("test2", "bar2")]))

    print "Testing that we can't rename a table so as to cause a name collision..."
    res = r.table_config("bar2").update({"name": "foo"}).run(conn)
    assert res["errors"] == 1

    print "Testing that we can create a table through table_config..."
    res = r.db("rethinkdb").table("table_config").insert({
        "name": "baz",
        "db": "test",
        "primary_key": "frob",
        "shards": [{"replicas": ["a"], "director": "a"}]
        }).run(conn)
    assert res["errors"] == 0, repr(res)
    assert res["inserted"] == 1, repr(res)
    assert "baz" in r.table_list().run(conn)
    for i in xrange(10):
        try:
            r.table("baz").insert({}).run(conn)
        except r.RqlRuntimeError:
            time.sleep(1)
        else:
            break
    else:
        raise ValueError("Table took too long to become available")
    rows = list(r.table("baz").run(conn))
    assert len(rows) == 1 and list(rows[0].keys()) == ["frob"]

    print "Testing that we can delete a table through table_config..."
    res = r.table_config("baz").delete().run(conn)
    assert res["errors"] == 0, repr(res)
    assert res["deleted"] == 1, repr(res)
    assert "baz" not in r.table_list().run(conn)

    cluster1.check_and_stop()
print "Done."

