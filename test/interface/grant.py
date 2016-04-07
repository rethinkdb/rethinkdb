#!/usr/bin/env python
# Copyright 2010-2016 RethinkDB, all rights reserved.

import os, sys, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, "common")))
import driver, utils

r = utils.import_python_driver()

with driver.Process() as process:
    conn = r.connect(process.host, process.driver_port)

    r.db("rethinkdb").table("users").insert({"id": "test", "password": False}).run(conn)

    try:
        res = r.grant("test", {"read": 1}).run(conn)
        assert False, res
    except r.ReqlOpFailedError, err:
        assert err.message == "Expected a boolean or null for `read`, got 1."

    try:
        res = r.grant("test", {"invalid": "invalid"}).run(conn)
        assert False, res
    except r.ReqlOpFailedError, err:
        assert err.message == "Unexpected key(s) `invalid`.", err

    res = r.grant(
            "test", {"read": True, "write": True, "config": True, "connect": True}
        ).run(conn)
    assert res["granted"] == 1, res
    assert len(res["permissions_changes"]) == 1, res
    assert res["permissions_changes"][0]["old_val"] == None, res
    assert res["permissions_changes"][0]["new_val"] == {
            "read": True, "write": True, "config": True, "connect": True
        }, res

    res = r.grant("test", {"read": False, "write": False}).run(conn)
    assert res["granted"] == 1, res
    assert len(res["permissions_changes"]) == 1, res
    assert res["permissions_changes"][0]["old_val"] == {
            "read": True, "write": True, "config": True, "connect": True
        }, res
    assert res["permissions_changes"][0]["new_val"] == {
            "read": False, "write": False, "config": True, "connect": True
        }, res

    res = r.grant("test", {"read": None}).run(conn)
    assert res["granted"] == 1, res
    assert len(res["permissions_changes"]) == 1, res
    assert res["permissions_changes"][0]["old_val"] == {
            "read": False, "write": False, "config": True, "connect": True
        }, res
    assert res["permissions_changes"][0]["new_val"] == {
            "write": False, "config": True, "connect": True
        }, res

    res = r.grant("test", {}).run(conn)
    assert res["granted"] == 1, res
    assert len(res["permissions_changes"]) == 1, res
    assert res["permissions_changes"][0]["old_val"] == \
            res["permissions_changes"][0]["new_val"], res

    res = r.grant(
            "test", {"read": None, "write": None, "config": None, "connect": None}
        ).run(conn)
    assert res["granted"] == 1, res
    assert len(res["permissions_changes"]) == 1, res
    assert res["permissions_changes"][0]["old_val"] == {
            "write": False, "config": True, "connect": True
        }, res
    assert res["permissions_changes"][0]["new_val"] == None, res

    database_id = r.db_create("test").run(conn)["config_changes"][0]["new_val"]["id"]

    try:
        res = r.db("test").grant("test", {"connect": True}).run(conn)
        assert False, res
    except r.ReqlOpFailedError, err:
        assert err.message == "The `connect` permission is only valid at the global scope.", err

    try:
        res = r.db("test").grant("test", {"invalid": "invalid"}).run(conn)
        assert False, res
    except r.ReqlOpFailedError, err:
        assert err.message == "Unexpected key(s) `invalid`.", err

    res = r.db("test").grant(
            "test", {"read": True, "write": True, "config": True}
        ).run(conn)
    assert res["granted"] == 1, res
    assert len(res["permissions_changes"]) == 1, res
    assert res["permissions_changes"][0]["old_val"] == None, res
    assert res["permissions_changes"][0]["new_val"] == {
            "read": True, "write": True, "config": True
        }, res

    table_id = r.db("test").table_create("test").run(conn)["config_changes"][0]["new_val"]["id"]

    try:
        res = r.db("test").table("test").grant("test", {"connect": True}).run(conn)
        assert False, res
    except r.ReqlOpFailedError, err:
        assert err.message == "The `connect` permission is only valid at the global scope.", err

    try:
        res = r.db("test").table("test").grant("test", {"invalid": "invalid"}).run(conn)
        assert False, res
    except r.ReqlOpFailedError, err:
        assert err.message == "Unexpected key(s) `invalid`.", err

    res = r.db("test").table("test").grant(
            "test", {"read": True, "write": True, "config": True}
        ).run(conn)
    assert res["granted"] == 1, res
    assert len(res["permissions_changes"]) == 1, res
    assert res["permissions_changes"][0]["old_val"] == None, res
    assert res["permissions_changes"][0]["new_val"] == {
            "read": True, "write": True, "config": True
        }, res

    try:
        res = r.grant("admin", {"config": False}).run(conn)
        assert False, res
    except r.ReqlOpFailedError, err:
        assert err.message == "The permissions of the user `admin` can't be modified.", err

    try:
        res = r.db("test").grant("admin", {"config": False}).run(conn)
        assert False, res
    except r.ReqlOpFailedError, err:
        assert err.message == "The permissions of the user `admin` can't be modified.", err

    try:
        res = r.db("test").table("test").grant("admin", {"config": False}).run(conn)
        assert False, res
    except r.ReqlOpFailedError, err:
        assert err.message == "The permissions of the user `admin` can't be modified.", err
