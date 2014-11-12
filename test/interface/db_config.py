#!/usr/bin/env python
# Copyright 2014 RethinkDB, all rights reserved.

import os, sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

r = utils.import_python_driver()

"""The `interface.db_config` test checks that the special `rethinkdb.db_config` table behaves as expected."""

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    
    print("Spinning up a process...")
    files = driver.Files(metacluster, console_output="create-output", server_name="a", command_prefix=command_prefix)
    proc = driver.Process(cluster, files, console_output="serve-output", command_prefix=command_prefix, extra_options=serve_options)
    proc.wait_until_started_up()
    cluster.check()
    conn = r.connect("localhost", proc.driver_port)

    assert list(r.db("rethinkdb").table("db_config").run(conn)) == []
    res = r.db_create("foo").run(conn)
    assert res == {"created":1}

    rows = list(r.db("rethinkdb").table("db_config").run(conn))
    assert len(rows) == 1 and rows[0]["name"] == "foo"
    foo_uuid = rows[0]["id"]
    assert r.db("rethinkdb").table("db_config").get(foo_uuid).run(conn)["name"] == "foo"

    res = r.db("rethinkdb").table("db_config").get(foo_uuid).update({"name": "foo2"}) \
           .run(conn)
    assert res["replaced"] == 1
    assert res["errors"] == 0
    rows = list(r.db("rethinkdb").table("db_config").run(conn))
    assert len(rows) == 1 and rows[0]["name"] == "foo2"

    res = r.db_create("bar").run(conn)
    assert res == {"created": 1}

    rows = list(r.db("rethinkdb").table("db_config").run(conn))
    assert len(rows) == 2 and set(row["name"] for row in rows) == set(["foo2", "bar"])
    bar_uuid = [row["id"] for row in rows if row["name"] == "bar"][0]

    rows = list(r.db_config().run(conn))
    assert len(rows) == 2 and set(row["name"] for row in rows) == set(["foo2", "bar"])

    rows = list(r.db_config("foo2").run(conn))
    assert len(rows) == 1 and rows[0]["name"] == "foo2"

    try:
        rows = r.db_config("not_a_database").run(conn)
    except r.RqlRuntimeError:
        pass
    else:
        raise ValueError("db_config() porcelain should fail if argument does not exist.")

    res = r.db("rethinkdb").table("db_config").get(bar_uuid).update({"name": "foo2"}) \
           .run(conn)
    # This would cause a name conflict, so it should fail
    assert res["errors"] == 1

    res = r.db("rethinkdb").table("db_config").insert({"name": "baz"}).run(conn)
    assert res["errors"] == 0
    assert res["inserted"] == 1
    assert "baz" in r.db_list().run(conn)
    baz_uuid = res["generated_keys"][0]

    res = r.db("rethinkdb").table("db_config").get(baz_uuid).delete().run(conn)
    assert res["errors"] == 0
    assert res["deleted"] == 1
    assert "baz" not in r.db_list().run(conn)

    cluster.check_and_stop()
print("Done.")

