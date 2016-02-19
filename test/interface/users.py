#!/usr/bin/env python
# Copyright 2010-2015 RethinkDB, all rights reserved.

import os, sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, "common")))
import driver, utils

r = utils.import_python_driver()

with driver.Process() as process:
    conn = r.connect(process.host, process.driver_port, auth_key="admin")
    users = r.db("rethinkdb").table("users")

    res = list(users.run(conn))
    assert res == [{"id": "admin", "password": False}], res
    res = users.get(None).run(conn)
    assert res == None, res
    res = users.get("admin").run(conn)
    assert res == {"id": "admin", "password": False}, res
    res = users.get("admin").delete().run(conn)
    assert res["errors"] == 1, res
    assert res["first_error"] == "The user `admin` can't be deleted", res
    res = users.get("admin").update({"password": "test"}).run(conn)
    assert res["replaced"] == 1, res
    res = users.get("admin").run(conn)
    assert res == {"id": "admin", "password": True}, res
    res = users.get("admin").update({"password": False}).run(conn)
    assert res["replaced"] == 1, res
    res = users.get("admin").run(conn)
    assert res == {"id": "admin", "password": False}, res
    res = users.get("admin").update({"password": None}).run(conn)
    assert res["errors"] == 1, res
    assert res["first_error"] == "Expected a string or boolean for `password`, got null", res
    res = users.get("admin").update({"password": True}).run(conn)
    assert res["errors"] == 1, res
    assert res["first_error"] == "No password is currently set, specify a string to set it or `false` to keep it unset, got true", res
    res = users.get("admin").replace({"id": "admin"}).run(conn)
    assert res["errors"] == 1, res
    assert res["first_error"] == "Expected a field named `password`", res
    res = users.get("admin").replace({"password": False}).run(conn)
    assert res["errors"] == 1, res
    assert res["first_error"] == "Inserted object must have primary key `id`:\n{\n\t\"password\":\tfalse\n}", res
    res = users.get("admin").replace({"id": "admin", "password": False, "test": "test"}).run(conn)
    assert res["errors"] == 1, res
    assert res["first_error"] == "Unexpected keys, got {\n\t\"id\":\t\"admin\",\n\t\"password\":\tfalse,\n\t\"test\":\t\"test\"\n}", res

    res = users.insert({"password": False}).run(conn)
    assert res["errors"] == 1, res
    # The `first_error` field contains a generated UUID, hence the `startswith`
    assert res["first_error"].startswith("Expected a username, got "), res
    res = users.insert({"id": "test", "password": None}).run(conn)
    assert res["errors"] == 1, res
    assert res["first_error"] == "Expected a string or boolean for `password`, got null", res
    res = users.insert({"id": "test", "password": True}).run(conn)
    assert res["errors"] == 1, res
    assert res["first_error"] == "No password is currently set, specify a string to set it or `false` to keep it unset, got true", res
    res = users.insert({"id": "test"}).run(conn)
    assert res["errors"] == 1, res
    assert res["first_error"] == "Expected a field named `password`", res
    res = users.insert({"id": "test", "password": False, "test": "test"}).run(conn)
    assert res["errors"] == 1, res
    assert res["first_error"] == "Unexpected keys, got {\n\t\"id\":\t\"test\",\n\t\"password\":\tfalse,\n\t\"test\":\t\"test\"\n}", res
    res = users.insert({"id": "test", "password": False}).run(conn)
    assert res["inserted"] == 1, res

    res = users.get("test").delete().run(conn)
    assert res["deleted"] == 1, res
    res = users.get("test").run(conn)
    assert res == None, res
    res = users.insert({"id": "test", "password": False}).run(conn)
    assert res["inserted"] == 1, res
    res = users.get("test").run(conn)
    assert res == {"id": "test", "password": False}, res
