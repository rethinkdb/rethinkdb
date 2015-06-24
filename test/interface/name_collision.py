#!/usr/bin/env python
# Copyright 2010-2015 RethinkDB, all rights reserved.

from __future__ import print_function

import os, pprint, random, string, sys, threading, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

opts = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(opts)
parsed_opts = opts.parse(sys.argv)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(parsed_opts)

r = utils.import_python_driver()

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)

    print("Starting first server and creating a database (%.2fs)" % (time.time() - startTime))
    files1 = driver.Files(metacluster, "my_server_name", db_path="server1_data",
        command_prefix=command_prefix, console_output=True)
    server1a = driver.Process(cluster, files1,
        command_prefix=command_prefix, extra_options=serve_options,
        wait_until_ready=True, console_output=True)
    conn1a = r.connect(host=server1a.host, port=server1a.driver_port)
    res = r.db_create("my_database_name").run(conn1a)
    db1_uuid = res["config_changes"][0]["new_val"]["id"]
    assert list(r.db("rethinkdb").table("current_issues").run(conn1a)) == []

    print("Stopping first server (%.2fs)" % (time.time() - startTime))
    server1a.check_and_stop()

    print("Starting second server and creating a database (%.2fs)" % (time.time() - startTime))
    files2 = driver.Files(metacluster, "my_server_name", db_path="server2_data",
        command_prefix=command_prefix, console_output=True)
    server2a = driver.Process(cluster, files2,
        command_prefix=command_prefix, extra_options=serve_options,
        wait_until_ready=True, console_output=True)
    conn2a = r.connect(host=server2a.host, port=server2a.driver_port)
    res = r.db_create("my_database_name").run(conn2a)
    db2_uuid = res["config_changes"][0]["new_val"]["id"]
    assert list(r.db("rethinkdb").table("current_issues").run(conn2a)) == []

    print("Restarting first server (%.2fs)" % (time.time() - startTime))
    server1b = driver.Process(cluster, files1,
        command_prefix=command_prefix, extra_options=serve_options,
        wait_until_ready=True, console_output=True)
    conn1b = r.connect(host=server1b.host, port=server1b.driver_port)

    print("Checking for server and DB name collision issues (%.2fs)" % (time.time() - startTime))
    issues = list(r.db("rethinkdb").table("current_issues").run(conn2a))
    try:
        assert len(issues) == 2
        if issues[0]["type"] == "server_name_collision":
            server_issue, db_issue = issues
        else:
            db_issue, server_issue = issues

        assert server_issue["type"] == "server_name_collision"
        assert server_issue["critical"]
        assert server_issue["info"]["name"] == "my_server_name"
        assert set(server_issue["info"]["ids"]) == set([server1b.uuid, server2a.uuid])

        assert db_issue["type"] == "db_name_collision"
        assert db_issue["critical"]
        assert db_issue["info"]["name"] == "my_database_name"
        assert set(db_issue["info"]["ids"]) == set([db1_uuid, db2_uuid])
    except Exception, e:
        pprint.pprint(issues)
        raise

    print("Resolving server and DB name collision issues (%.2fs)" % (time.time() - startTime))
    res = r.db("rethinkdb").table("server_config").get(server2a.uuid) \
        .update({"name": "my_server_name2"}).run(conn2a)
    assert res["replaced"] == 1 and res["errors"] == 0
    res = r.db("rethinkdb").table("db_config").get(db2_uuid) \
        .update({"name": "my_database_name2"}).run(conn2a)
    assert res["replaced"] == 1 and res["errors"] == 0

    print("Confirming that server and DB name collision issues are gone (%.2fs)" % (time.time() - startTime))
    issues = list(r.db("rethinkdb").table("current_issues").run(conn2a))
    assert len(issues) == 0, issues

    print("Stopping second server (%.2fs)" % (time.time() - startTime))
    server2a.check_and_stop()

    print("Creating a table on first server (%.2fs)" % (time.time() - startTime))
    res = r.db("my_database_name").table_create("my_table_name").run(conn1b)
    table1_uuid = res["config_changes"][0]["new_val"]["id"]
    assert list(r.db("rethinkdb").table("current_issues").run(conn1b)) == []

    print("Stopping first server (%.2fs)" % (time.time() - startTime))
    server1b.check_and_stop()

    print("Restarting second server (%.2fs)" % (time.time() - startTime))
    server2b = driver.Process(cluster, files2,
        command_prefix=command_prefix, extra_options=serve_options,
        wait_until_ready=True, console_output=True)
    conn2b = r.connect(host=server2b.host, port=server2b.driver_port)

    print("Creating a table on second server (%.2fs)" % (time.time() - startTime))
    res = r.db("my_database_name").table_create("my_table_name").run(conn2b)
    table2_uuid = res["config_changes"][0]["new_val"]["id"]
    assert list(r.db("rethinkdb").table("current_issues").run(conn2b)) == []

    print("Restarting first server (%.2fs)" % (time.time() - startTime))
    server1c = driver.Process(cluster, files1,
        command_prefix=command_prefix, extra_options=serve_options,
        wait_until_ready=True, console_output=True)
    conn1c = r.connect(host=server1c.host, port=server1c.driver_port)

    print("Checking for table name collision issue (%.2fs)" % (time.time() - startTime))
    issues = list(r.db("rethinkdb").table("current_issues").run(conn1c))
    try:
        assert len(issues) == 2
        if issues[0]["type"] == "table_name_collision":
            table_issue, availability_issue = issues
        else:
            availability_issue, table_issue = issues
        assert availability_issue["type"] == "table_availability"

        assert table_issue["type"] == "table_name_collision"
        assert table_issue["critical"]
        assert table_issue["info"]["name"] == "my_table_name"
        assert table_issue["info"]["db"] == "my_database_name"
        assert set(table_issue["info"]["ids"]) == set([table1_uuid, table2_uuid])
    except Exception, e:
        pprint.pprint(issues)
        raise

    print("Resolving table name collision issue (%.2fs)" % (time.time() - startTime))
    res = r.db("rethinkdb").table("table_config").get(table2_uuid) \
        .update({"name": "my_table_name2"}).run(conn1c)
    assert res["replaced"] == 1 and res["errors"] == 0

    print("Confirming that table collision issue are gone (%.2fs)" % (time.time() - startTime))
    r.db("my_database_name").table("my_table_name").wait().run(conn1c)
    r.db("my_database_name").table("my_table_name2").wait().run(conn1c)
    issues = list(r.db("rethinkdb").table("current_issues").run(conn1c))
    assert len(issues) == 0, issues

    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))

