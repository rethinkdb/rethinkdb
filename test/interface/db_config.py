#!/usr/bin/env python
# Copyright 2014 RethinkDB, all rights reserved.

"""The `interface.db_config` test checks that the special `rethinkdb.db_config` table behaves as expected."""

from __future__ import print_function

import os, sys, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

r = utils.import_python_driver()

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

print("Starting server (%.2fs)" % (time.time() - startTime))
with driver.Process(files='a', output_folder='.', command_prefix=command_prefix, extra_options=serve_options, wait_until_ready=True) as server:
    
    print("Establishing ReQL connection (%.2fs)" % (time.time() - startTime))
    
    conn = r.connect(host=server.host, port=server.driver_port)
    
    print("Starting tests (%.2fs)" % (time.time() - startTime))

    assert list(r.db("rethinkdb").table("db_config").run(conn)) == []
    res = r.db_create("foo").run(conn)
    assert res["dbs_created"] == 1
    assert len(res["config_changes"]) == 1
    assert res["config_changes"][0]["old_val"] is None
    assert res["config_changes"][0]["new_val"] == r.db("foo").config().run(conn)

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
    assert res["dbs_created"] == 1

    rows = list(r.db("rethinkdb").table("db_config").run(conn))
    assert len(rows) == 2 and set(row["name"] for row in rows) == set(["foo2", "bar"])
    bar_uuid = [row["id"] for row in rows if row["name"] == "bar"][0]

    foo2_config = r.db("foo2").config().run(conn)
    assert foo2_config["name"] == "foo2"

    try:
        rows = r.db("not_a_database").config().run(conn)
    except r.RqlRuntimeError:
        pass
    else:
        raise ValueError("r.db().config() should fail if argument does not exist.")

    res = r.db("rethinkdb").table("db_config").get(bar_uuid).update({"name": "foo2"}) \
           .run(conn)
    # This would cause a name conflict, so it should fail
    assert res["errors"] == 1

    res = r.db_drop("foo2").run(conn)
    assert res["dbs_dropped"] == 1
    assert res["tables_dropped"] == 0
    assert len(res["config_changes"]) == 1
    assert res["config_changes"][0]["old_val"] == foo2_config
    assert res["config_changes"][0]["new_val"] is None

    res = r.db("rethinkdb").table("db_config").insert({"name": "baz"}).run(conn)
    assert res["errors"] == 0
    assert res["inserted"] == 1
    assert "baz" in r.db_list().run(conn)
    baz_uuid = res["generated_keys"][0]

    res = r.db("rethinkdb").table("db_config").get(baz_uuid).delete().run(conn)
    assert res["errors"] == 0
    assert res["deleted"] == 1
    assert "baz" not in r.db_list().run(conn)

    print("Inserting nonsense to make sure it's not accepted (%.2fs)" %
        (time.time() - startTime))
    res = r.db("rethinkdb").table("db_config").insert({}).run(conn)
    assert res["errors"] == 1, res
    res = r.db("rethinkdb").table("db_config") \
           .insert({"name": "hi", "nonsense": "yes"}).run(conn)
    assert res["errors"] == 1, res

    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))
