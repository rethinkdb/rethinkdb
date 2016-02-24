#!/usr/bin/env python
# Copyright 2010-2016 RethinkDB, all rights reserved.

import os, sys, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, "common")))
import driver, utils

r = utils.import_python_driver()

with driver.Process() as process:
    conn = r.connect(process.host, process.driver_port, auth_key="admin")

    r.db("rethinkdb").table("users").insert({"id": "test", "password": False}).run(conn)

    r.grant(
            "test", {"read": True, "write": True, "config": True, "connect": True}
        ).run(conn)

    database_id = r.db_create("test").run(conn)["config_changes"][0]["new_val"]["id"]
    r.db("test").grant("test", {"read": True, "write": True, "config": True}).run(conn)

    table_id = r.db("test").table_create("test").run(conn)["config_changes"][0]["new_val"]["id"]
    r.db("test").table("test").grant(
            "test", {"read": True, "write": True, "config": True}
        ).run(conn)


    permissions = r.db("rethinkdb").table("permissions")

    res = list(permissions.run(conn))
    assert res == [
        {
            "id": ["admin"],
            "permissions": {"read": True, "write": True, "config": True, "connect": True}
        },
        {
            "id": ["test"],
            "permissions": {"read": True, "write": True, "config": True, "connect": True}
        },
        {
            "database": "test",
            "id": ["test", database_id],
            "permissions": {"read": True, "write": True, "config": True}
        },
        {
            "database": "test",
            "id": ["test", database_id, table_id],
            "permissions": {"read": True, "write": True, "config": True},
            "table": "test"
        }
    ], res
    res = permissions.get([]).run(conn)
    assert res == None, res
    res = permissions.get("invalid").run(conn)
    assert res == None, res
    res = permissions.get(["invalid"]).run(conn)
    assert res == None, res
    res = permissions.get(["test"]).run(conn)
    assert res == {
        "id": ["test"],
        "permissions": {"read": True, "write": True, "config": True, "connect": True}
    }, res
    res = permissions.get(["invalid", database_id]).run(conn)
    assert res == None, res
    res = permissions.get(["invalid", "invalid"]).run(conn)
    assert res == None, res
    res = permissions.get(["invalid", "test"]).run(conn)
    assert res == None, res
    res = permissions.get(["test", database_id]).run(conn)
    assert res == {
        "database": "test",
        "id": ["test", database_id],
        "permissions": {"read": True, "write": True, "config": True}
    }, res
    res = permissions.get(["test", "invalid"]).run(conn)
    assert res == None, res
    res = permissions.get(["invalid", "invalid", "invalid"]).run(conn)
    assert res == None, res
    res = permissions.get(["invalid", database_id, "invalid"]).run(conn)
    assert res == None, res
    res = permissions.get(["invalid", "invalid", table_id]).run(conn)
    assert res == None, res
    res = permissions.get(["invalid", database_id, table_id]).run(conn)
    assert res == None, res
    res = permissions.get(["test", "invalid", "invalid"]).run(conn)
    assert res == None, res
    res = permissions.get(["test", database_id, "invalid"]).run(conn)
    assert res == None, res
    res = permissions.get(["test", "invalid", table_id]).run(conn)
    assert res == None, res
    res = permissions.get(["test", database_id, table_id]).run(conn)
    assert res == {
        "database": "test",
        "id": ["test", database_id, table_id],
        "permissions": {"read": True, "write": True, "config": True},
        "table": "test"
    }, res
    res = permissions.get(["test", database_id]).run(conn, identifier_format="uuid")
    assert res == {
        "database": database_id,
        "id": ["test", database_id],
        "permissions": {"read": True, "write": True, "config": True}
    }, res
    res = permissions.get(["test", database_id, table_id]).run(
        conn, identifier_format="uuid")
    assert res == {
        "database": database_id,
        "id": ["test", database_id, table_id],
        "permissions": {"read": True, "write": True, "config": True},
        "table": table_id
    }, res
    res = permissions.get(["test", database_id, table_id]).update(
            {"permissions": {"read": False, "write": False, "config": False}}
        ).run(conn)
    assert res["replaced"] == 1, res
    res = permissions.get(["test", database_id, table_id]).update(
            {"permissions": {"read": None, "write": None, "config": None}}
        ).run(conn)
    assert res["deleted"] == 1, res
    res = permissions.get(["test", database_id, table_id]).update(
            {"permissions": {"read": True, "write": True, "config": True}}
        ).run(conn)

    r.db("test").table_drop("test").run(conn)

    res = list(permissions.run(conn))
    assert res == [
        {
            "id": ["admin"],
            "permissions": {"read": True, "write": True, "config": True, "connect": True}
        },
        {
            "id": ["test"],
            "permissions": {"read": True, "write": True, "config": True, "connect": True}
        },
        {
            "database": "test",
            "id": ["test", database_id],
            "permissions": {"read": True, "write": True, "config": True}
        }
    ], res

    r.db_drop("test").run(conn)

    res = list(permissions.run(conn))
    assert res == [
        {
            "id": ["admin"],
            "permissions": {"read": True, "write": True, "config": True, "connect": True}
        },
        {
            "id": ["test"],
            "permissions": {"read": True, "write": True, "config": True, "connect": True}
        },
        {
            "database": "__deleted_database__",
            "id": ["test", database_id],
            "permissions": {"read": True, "write": True, "config": True}
        }
    ], res

    r.db("rethinkdb").table("users").get("test").delete().run(conn)

    res = list(permissions.run(conn))
    assert res == [
        {
            "id": ["admin"],
            "permissions": {"read": True, "write": True, "config": True, "connect": True}
        }
    ], res

    r.db("rethinkdb").table("users").insert({"id": "test", "password": False}).run(conn)
    database_id = r.db_create("test").run(conn)["config_changes"][0]["new_val"]["id"]
    table_id = r.db("test").table_create("test").run(conn)["config_changes"][0]["new_val"]["id"]

    res = permissions.insert({}).run(conn)
    assert res["errors"] == 1, res
    assert res["first_error"] == "You must specify a primary key", res
    res = permissions.insert({"id": []}).run(conn)
    assert res["errors"] == 1, res
    assert res["first_error"] == "Expected an array of one to three items, got []", res
    res = permissions.insert({"id": [False, False, False, False]}).run(conn)
    assert res["errors"] == 1, res
    assert res["first_error"] == "Expected an array of one to three items, got [\n\tfalse,\n\tfalse,\n\tfalse,\n\tfalse\n]", res
    res = permissions.insert({"id": [False]}).run(conn)
    assert res["errors"] == 1, res
    assert res["first_error"] == "Expected a string as the username, got false", res
    res = permissions.insert({"id": ["invalid"]}).run(conn)
    assert res["errors"] == 1, res
    assert res["first_error"] == "No user named `invalid`", res
    res = permissions.insert({"id": ["test", "invalid"]}).run(conn)
    assert res["errors"] == 1, res
    assert res["first_error"] == "Expected a UUID as the database, got \"invalid\"", res
    res = permissions.insert(
            {
                "id": ["test", "00000000-0000-0000-0000-000000000000"]
            }
        ).run(conn)
    assert res["errors"] == 1, res
    assert res["first_error"] == "No database with UUID `00000000-0000-0000-0000-000000000000` exists", res
    res = permissions.insert({"id": ["test", database_id, "invalid"]}).run(conn)
    assert res["errors"] == 1, res
    assert res["first_error"] == "Expected a UUID as the table, got \"invalid\"", res
    res = permissions.insert(
            {
                "id": ["test", database_id, "00000000-0000-0000-0000-000000000000"]
            }
        ).run(conn)
    assert res["errors"] == 1, res
    assert res["first_error"] == "No table with UUID `00000000-0000-0000-0000-000000000000` exists", res
    res = permissions.insert({"id": ["test", database_id, table_id]}).run(conn)
    assert res["errors"] == 1, res
    assert res["first_error"] == "Expected a field `permissions`", res
    res = permissions.insert(
            {
                "id": ["test", database_id, table_id],
                "permissions": {},
                "invalid": "invalid"
            }
        ).run(conn)
    assert res["errors"] == 1, res
    assert res["first_error"] == "Unexpected keys, got {\n\t\"id\":\t[\n\t\t\"test\",\n\t\t\"%s\",\n\t\t\"%s\"\n\t],\n\t\"invalid\":\t\"invalid\",\n\t\"permissions\":\t{}\n}" % (database_id, table_id), res
    res = permissions.insert(
            {
                "database": "invalid",
                "id": ["test", database_id],
                "permissions": {}
            }
        ).run(conn)
    assert res["errors"] == 1, res
    assert res["first_error"] == "The key `database` does not match the primary key", res
    res = permissions.insert(
            {
                "database": "test",
                "id": ["test", database_id, table_id],
                "permissions": {},
                "table": "invalid"
            }
        ).run(conn)
    assert res["errors"] == 1, res
    assert res["first_error"] == "The key `table` does not match the primary key", res
