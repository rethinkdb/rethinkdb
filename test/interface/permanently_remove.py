#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

from __future__ import print_function

import pprint, os, socket, sys, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

r = utils.import_python_driver()
dbName, _ = utils.get_test_db_table()

print("Starting servers PrinceHamlet and KingHamlet (%.2fs)" % (time.time() - startTime))
with driver.Cluster(output_folder='.') as cluster:
    
    prince_hamlet = driver.Process(cluster=cluster, files='PrinceHamlet', command_prefix=command_prefix, extra_options=serve_options)
    king_hamlet = driver.Process(cluster=cluster, files='KingHamlet', command_prefix=command_prefix, extra_options=serve_options)
    king_hamlet_files = king_hamlet.files
    
    cluster.wait_until_ready()
    cluster.check()
    
    print("Establishing ReQL connection (%.2fs)" % (time.time() - startTime))
    
    conn = r.connect(prince_hamlet.host, prince_hamlet.driver_port)

    print("Creating three tables (%.2fs)" % (time.time() - startTime))
    
    if dbName not in r.db_list().run(conn):
        r.db_create(dbName).run(conn)
    
    res = r.db("rethinkdb").table("table_config").insert([
        # The `test` table will remain readable when `KingHamlet` is removed.
        {
            "db": dbName,
            "name": "test",
            "primary_key": "id",
            "shards": [{
                "primary_replica": "PrinceHamlet",
                "replicas": ["PrinceHamlet", "KingHamlet"]
                }],
            "write_acks": "single"
        },
        # The `test2` table will raise a `table_needs_primary` issue
        {
            "db": dbName,
            "name": "test2",
            "primary_key": "id",
            "shards": [{
                "primary_replica": "KingHamlet",
                "replicas": ["PrinceHamlet", "KingHamlet"]
                }],
            "write_acks": "single"
        },
        # The `test3` table will raise a `data_lost` issue
        {
            "db": dbName,
            "name": "test3",
            "primary_key": "id",
            "shards": [{
                "primary_replica": "KingHamlet",
                "replicas": ["KingHamlet"]
                }],
            "write_acks": "single"
        }
        ]).run(conn)
    assert res["inserted"] == 3, res
    r.db(dbName).wait().run(conn)
    
    print("Inserting data into tables (%.2fs)" % (time.time() - startTime))
    
    res = r.db(dbName).table("test").insert([{}]*100).run(conn)
    assert res["inserted"] == 100
    res = r.db(dbName).table("test2").insert([{}]*100).run(conn)
    assert res["inserted"] == 100
    res = r.db(dbName).table("test3").insert([{}]*100).run(conn)
    assert res["inserted"] == 100

    print("Killing KingHamlet (%.2fs)" % (time.time() - startTime))
    king_hamlet.close()
    time.sleep(1)
    cluster.check()

    print("Checking that the other shows an issue (%.2fs)" % (time.time() - startTime))
    issues = list(r.db("rethinkdb").table("issues").run(conn))
    pprint.pprint(issues)
    assert len(issues) == 1, issues
    assert issues[0]["type"] == "server_down"
    assert issues[0]["critical"]
    assert "KingHamlet" in issues[0]["description"]
    assert issues[0]["info"]["server"] == "KingHamlet"
    assert issues[0]["info"]["affected_servers"] == ["PrinceHamlet"]
    
    # identifier_format='uuid'
    issues = list(r.db("rethinkdb").table("issues", identifier_format='uuid').run(conn))
    assert issues[0]["info"]["server"] == king_hamlet.uuid
    assert issues[0]["info"]["affected_servers"] == [prince_hamlet.uuid]

    test_status = r.db(dbName).table("test").status().run(conn)
    test2_status = r.db(dbName).table("test2").status().run(conn)
    test3_status = r.db(dbName).table("test3").status().run(conn)
    assert test_status["status"]["ready_for_writes"], test_status
    assert not test_status["status"]["all_replicas_ready"], test_status
    assert test2_status["status"]["ready_for_outdated_reads"], test2_status
    assert not test2_status["status"]["ready_for_reads"], test2_status
    assert not test3_status["status"]["ready_for_outdated_reads"], test3_status

    print("Permanently removing KingHamlet (%.2fs)" % (time.time() - startTime))
    res = r.db("rethinkdb").table("server_config").filter({"name": "KingHamlet"}).delete().run(conn)
    assert res["deleted"] == 1
    assert res["errors"] == 0

    print("Checking the issues that were generated (%.2fs)" % (time.time() - startTime))
    issues = list(r.db("rethinkdb").table("issues").run(conn))
    assert len(issues) == 2, issues
    if issues[0]["type"] == "data_lost":
        dl_issue, np_issue = issues
    else:
        np_issue, dl_issue = issues

    assert np_issue["type"] == "table_needs_primary"
    assert np_issue["info"]["table"] == "test2"
    assert "no primary replica" in np_issue["description"]

    assert dl_issue["type"] == "data_lost"
    assert dl_issue["info"]["table"] == "test3"
    assert "Some data has probably been lost permanently" in dl_issue["description"]

    test_status = r.db(dbName).table("test").status().run(conn)
    test2_status = r.db(dbName).table("test2").status().run(conn)
    test3_status = r.db(dbName).table("test3").status().run(conn)
    assert test_status["status"]["all_replicas_ready"]
    assert test2_status["status"]["ready_for_outdated_reads"]
    assert not test2_status["status"]["ready_for_reads"]
    assert not test3_status["status"]["ready_for_outdated_reads"]
    assert r.db(dbName).table("test").config()["shards"].run(conn) == [{
        "primary_replica": "PrinceHamlet",
        "replicas": ["PrinceHamlet"]
        }]
    assert r.db(dbName).table("test2").config()["shards"].run(conn) == [{
        "primary_replica": None,
        "replicas": ["PrinceHamlet"]
        }]
    assert r.db(dbName).table("test3").config()["shards"].run(conn) == [{
        "primary_replica": None,
        "replicas": []
        }]

    print("Testing that having primary_replica=None doesn't break `table_config` (%.2fs)" % (time.time() - startTime))
    # By changing the table's name, we force a write to `table_config`, which tests the
    # code path that writes `"primary_replica": None`.
    res = r.db(dbName).table("test2").config().update({"name": "test2x"}).run(conn)
    assert res["errors"] == 0
    res = r.db(dbName).table("test2x").config().update({"name": "test2"}).run(conn)
    assert res["errors"] == 0
    assert r.db(dbName).table("test2").config()["shards"].run(conn) == [{
        "primary_replica": None,
        "replicas": ["PrinceHamlet"]
        }]

    print("Fixing table `test2` (%.2fs)" % (time.time() - startTime))
    r.db(dbName).table("test2").reconfigure(shards=1, replicas=1).run(conn)
    r.db(dbName).table("test2").wait().run(conn)

    print("Fixing table `test3` (%.2fs)" % (time.time() - startTime))
    r.db(dbName).table("test3").reconfigure(shards=1, replicas=1).run(conn)
    r.db(dbName).table("test3").wait().run(conn)

    print("Bringing the dead server back as a ghost (%.2fs)" % (time.time() - startTime))
    ghost_of_king_hamlet = driver.Process(cluster, king_hamlet_files, console_output="king-hamlet-ghost-log", command_prefix=command_prefix)
    ghost_of_king_hamlet.wait_until_started_up()
    cluster.check()

    print("Checking that there is an issue (%.2fs)" % (time.time() - startTime))
    issues = list(r.db("rethinkdb").table("issues").run(conn))
    pprint.pprint(issues)
    assert len(issues) == 1, issues
    assert issues[0]["type"] == "server_ghost"
    assert not issues[0]["critical"]
    assert issues[0]["info"]["server_id"] == king_hamlet.uuid
    assert issues[0]["info"]["hostname"] == socket.gethostname()
    assert issues[0]["info"]["pid"] == ghost_of_king_hamlet.process.pid

    print("Checking table contents (%.2fs)" % (time.time() - startTime))
    assert r.db(dbName).table("test").count().run(conn) == 100
    assert r.db(dbName).table("test2").count().run(conn) == 100
    assert r.db(dbName).table("test3").count().run(conn) == 0

    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))
