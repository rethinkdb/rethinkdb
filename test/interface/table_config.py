#!/usr/bin/env python
# Copyright 2014 RethinkDB, all rights reserved.

"""The `interface.table_config` test checks that the special `rethinkdb.table_config` and `rethinkdb.table_status` tables behave as expected."""

from __future__ import print_function

import os, pprint, sys, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

r = utils.import_python_driver()
dbName, _ = utils.get_test_db_table()

print("Starting cluster of %d servers (%.2fs)" % (3, time.time() - startTime))
with driver.Cluster(initial_servers=['a', 'b', 'never_used'], output_folder='.', command_prefix=command_prefix, extra_options=serve_options, wait_until_ready=True) as cluster:
    
    print("Establishing ReQL connection (%.2fs)" % (time.time() - startTime))
    
    server = cluster[0]
    conn = r.connect(host=server.host, port=server.driver_port)
    
    def check_foo_config_matches(expected):
        config = r.db(dbName).table("foo").config().run(conn)
        assert config["name"] == "foo" and config["db"] == "test"
        found = config["shards"]
        if len(expected) != len(found):
            return False
        for (e_shard, f_shard) in zip(expected, found):
            if set(e_shard["replicas"]) != set(f_shard["replicas"]):
                return False
            if e_shard["primary_replica"] != f_shard["primary_replica"]:
                return False
        return True

    def check_status_matches_config():
        config = list(r.db("rethinkdb").table("table_config").run(conn))
        status = list(r.db("rethinkdb").table("table_status").run(conn))
        uuids = set(row["id"] for row in config)
        if not (len(uuids) == len(config) == len(status)):
            return False
        if uuids != set(row["id"] for row in status):
            return False
        for c_row in config:
            s_row = [row for row in status if row["id"] == c_row["id"]][0]
            if c_row["db"] != s_row["db"]:
                return False
            if c_row["name"] != s_row["name"]:
                return False
            c_shards = c_row["shards"]
            s_shards = s_row["shards"]
            # Make sure that servers that have never been involved with the table will
            # never appear in `table_status`. (See GitHub issue #3101.)
            for s_shard in s_shards:
                for doc in s_shard["replicas"]:
                    assert doc["server"] != "never_used"
            if len(s_shards) != len(c_shards):
                return False
            for (s_shard, c_shard) in zip(s_shards, c_shards):
                if set(doc["server"] for doc in s_shard["replicas"]) != \
                        set(c_shard["replicas"]):
                    return False
                if s_shard["primary_replica"] != c_shard["primary_replica"]:
                    return False
                if any(doc["state"] != "ready" for doc in s_shard["replicas"]):
                    return False
            if not s_row["status"]["ready_for_outdated_reads"]:
                return False
            if not s_row["status"]["ready_for_reads"]:
                return False
            if not s_row["status"]["ready_for_writes"]:
                return False
            if not s_row["status"]["all_replicas_ready"]:
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
            print("Something went wrong.\nconfig =")
            pprint.pprint(config)
            print("status =")
            pprint.pprint(status)
            raise

    # Make sure that `never_used` will never be picked as a default for a table by removing the `default` tag.
    
    res = r.db("rethinkdb").table("server_config").filter({"name": "never_used"}).update({"tags": []}).run(conn)
    assert res["replaced"] == 1, res

    print("Creating dbs and tables (%.2fs)" % (time.time() - startTime))
    
    if dbName not in r.db_list().run(conn):
        r.db_create(dbName).run(conn)

    res = r.db(dbName).table_create("foo").run(conn)
    assert res["tables_created"] == 1
    assert len(res["config_changes"]) == 1
    assert res["config_changes"][0]["old_val"] is None
    assert res["config_changes"][0]["new_val"] == \
        r.db(dbName).table("foo").config().run(conn)

    r.db(dbName).table_create("bar").run(conn)
    
    if "test2" not in r.db_list().run(conn):
        r.db_create("test2").run(conn)
    r.db("test2").table_create("bar2").run(conn)
    
    print("Inserting data into foo (%.2fs)" % (time.time() - startTime))
    
    r.db(dbName).table("foo").insert([{"i": i} for i in xrange(10)]).run(conn)
    assert set(row["i"] for row in r.db(dbName).table("foo").run(conn)) == set(xrange(10))

    print("Testing that table_config and table_status are sane (%.2fs)" % (time.time() - startTime))
    wait_until(lambda: check_tables_named(
        [("test", "foo"), ("test", "bar"), ("test2", "bar2")]))
    wait_until(check_status_matches_config)

    print("Testing that we can move around data by writing to table_config (%.2fs)" % (time.time() - startTime))
    def test_shards(shards):
        print("Reconfiguring:", {"shards": shards})
        res = r.db(dbName).table("foo").config().update({"shards": shards}).run(conn)
        assert res["errors"] == 0, repr(res)
        wait_until(lambda: check_foo_config_matches(shards))
        wait_until(check_status_matches_config)
        assert set(row["i"] for row in r.db(dbName).table("foo").run(conn)) == set(xrange(10))
        print("OK (%.2fs)" % (time.time() - startTime))
    test_shards([{"replicas": ["a"], "primary_replica": "a"}])
    test_shards([{"replicas": ["b"], "primary_replica": "b"}])
    test_shards([{"replicas": ["a", "b"], "primary_replica": "a"}])
    test_shards([
        {"replicas": ["a"], "primary_replica": "a"},
        {"replicas": ["b"], "primary_replica": "b"}
    ])
    test_shards([
        {"replicas": ["a", "b"], "primary_replica": "a"},
        {"replicas": ["a", "b"], "primary_replica": "b"}
    ])
    test_shards([{"replicas": ["a"], "primary_replica": "a"}])

    print("Testing that table_config rejects invalid input (%.2fs)" % (time.time() - startTime))
    def test_invalid(conf):
        print("Reconfiguring:", conf)
        res = r.db("rethinkdb").table("table_config").filter({"name": "foo"}).replace(conf).run(conn)
        assert res["errors"] == 1
        print("Error, as expected")
    test_invalid(r.row.merge({"shards": []}))
    test_invalid(r.row.merge({"shards": "this is a string"}))
    test_invalid(r.row.merge({"shards":
        [{"replicas": ["a"], "primary_replica": "a", "extra_key": "extra_value"}]}))
    test_invalid(r.row.merge({"shards": [{"replicas": [], "primary_replica": None}]}))
    test_invalid(r.row.merge({"shards": [{"replicas": ["a"], "primary_replica": "b"}]}))
    test_invalid(r.row.merge(
        {"shards": [{"replicas": ["a"], "primary_replica": "b"},
                    {"replicas": ["b"], "primary_replica": "a"}]}))
    test_invalid(r.row.merge({"primary_key": "new_primary_key"}))
    test_invalid(r.row.merge({"db": "new_db"}))
    test_invalid(r.row.merge({"extra_key": "extra_value"}))
    test_invalid(r.row.without("name"))
    test_invalid(r.row.without("primary_key"))
    test_invalid(r.row.without("db"))
    test_invalid(r.row.without("shards"))

    print("Testing that we can rename tables through table_config (%.2fs)" % (time.time() - startTime))
    res = r.db(dbName).table("bar").config().update({"name": "bar2"}).run(conn)
    assert res["errors"] == 0
    wait_until(lambda: check_tables_named(
        [("test", "foo"), ("test", "bar2"), ("test2", "bar2")]))

    print("Testing that we can't rename a table so as to cause a name collision (%.2fs)" % (time.time() - startTime))
    res = r.db(dbName).table("bar2").config().update({"name": "foo"}).run(conn)
    assert res["errors"] == 1

    print("Testing table_drop() (%.2fs)" % (time.time() - startTime))
    foo_config = r.db(dbName).table("foo").config().run(conn)
    res = r.db(dbName).table_drop("foo").run(conn)
    assert res["tables_dropped"] == 1
    assert len(res["config_changes"]) == 1
    assert res["config_changes"][0]["old_val"] == foo_config
    assert res["config_changes"][0]["new_val"] is None

    print("Testing that we can create a table through table_config (%.2fs)" % (time.time() - startTime))
    def test_create(doc, pkey):
        res = r.db("rethinkdb").table("table_config") \
               .insert(doc, return_changes=True).run(conn)
        assert res["errors"] == 0, repr(res)
        assert res["inserted"] == 1, repr(res)
        assert doc["name"] in r.db(dbName).table_list().run(conn)
        assert res["changes"][0]["new_val"]["primary_key"] == pkey
        assert "shards" in res["changes"][0]["new_val"]
        for i in xrange(10):
            try:
                r.db(dbName).table(doc["name"]).insert({}).run(conn)
            except r.RqlRuntimeError:
                time.sleep(1)
            else:
                break
        else:
            raise ValueError("Table took too long to become available")
        rows = list(r.db(dbName).table(doc["name"]).run(conn))
        assert len(rows) == 1 and list(rows[0].keys()) == [pkey]
    test_create({
        "name": "baz",
        "db": "test",
        "primary_key": "frob",
        "shards": [{"replicas": ["a"], "primary_replica": "a"}]
        }, "frob")
    test_create({
        "name": "baz2",
        "db": "test",
        "shards": [{"replicas": ["a"], "primary_replica": "a"}]
        }, "id")
    test_create({
        "name": "baz3",
        "db": "test"
        }, "id")

    print("Testing that we can delete a table through table_config (%.2fs)" % (time.time() - startTime))
    res = r.db(dbName).table("baz").config().delete().run(conn)
    assert res["errors"] == 0, repr(res)
    assert res["deleted"] == 1, repr(res)
    assert "baz" not in r.db(dbName).table_list().run(conn)

    print("Testing that identifier_format works (%.2fs)" % (time.time() - startTime))
    a_uuid = r.db("rethinkdb").table("server_config") \
              .filter({"name": "a"}).nth(0)["id"].run(conn)
    db_uuid = r.db("rethinkdb").table("db_config") \
               .filter({"name": "test"}).nth(0)["id"].run(conn)
    res = r.db("rethinkdb").table("table_config", identifier_format="uuid") \
           .insert({
               "name": "idf_test",
               "db": db_uuid,
               "shards": [{"replicas": [a_uuid], "primary_replica": a_uuid}]
               }) \
           .run(conn)
    assert res["inserted"] == 1, repr(res)
    res = r.db("rethinkdb").table("table_config", identifier_format="uuid") \
           .filter({"name": "idf_test"}).nth(0).run(conn)
    assert res["shards"] == [{"replicas": [a_uuid], "primary_replica": a_uuid}], repr(res)
    res = r.db("rethinkdb").table("table_config", identifier_format="name") \
           .filter({"name": "idf_test"}).nth(0).run(conn)
    assert res["shards"] == [{"replicas": ["a"], "primary_replica": "a"}], repr(res)
    r.db(dbName).table("idf_test").wait().run(conn)
    res = r.db("rethinkdb").table("table_status", identifier_format="uuid") \
           .filter({"name": "idf_test"}).nth(0).run(conn)
    assert res["shards"] == [{
        "replicas": [{"server": a_uuid, "state": "ready"}],
        "primary_replica": a_uuid
        }], repr(res)
    res = r.db("rethinkdb").table("table_status", identifier_format="name") \
           .filter({"name": "idf_test"}).nth(0).run(conn)
    assert res["shards"] == [{
        "replicas": [{"server": "a", "state": "ready"}],
        "primary_replica": "a"
        }], repr(res)

    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))
