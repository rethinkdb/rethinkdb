#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.
import sys, os, time, pprint, socket
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, http_admin, scenario_common, utils, vcoptparse
r = utils.import_python_driver()

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = \
        scenario_common.parse_mode_flags(opts)
    print("Spinning up two processes...")
    prince_hamlet_files = driver.Files(metacluster, server_name = "PrinceHamlet",
        db_path = "prince-hamlet-db", console_output = "prince-hamlet-create-output",
        executable_path = executable_path, command_prefix = command_prefix)
    prince_hamlet = driver.Process(cluster, prince_hamlet_files,
        console_output = "prince-hamlet-log", executable_path = executable_path,
        command_prefix = command_prefix, extra_options = serve_options)
    king_hamlet_files = driver.Files(metacluster, server_name = "KingHamlet",
        db_path = "king-hamlet-db", console_output = "king-hamlet-create-output",
        executable_path = executable_path, command_prefix = command_prefix)
    king_hamlet = driver.Process(cluster, king_hamlet_files,
        console_output = "king-hamlet-log", executable_path = executable_path,
        command_prefix = command_prefix, extra_options = serve_options)
    prince_hamlet.wait_until_started_up()
    king_hamlet.wait_until_started_up()
    cluster.check()
    conn = r.connect(prince_hamlet.host, prince_hamlet.driver_port)
    king_hamlet_id = r.db("rethinkdb").table("server_config") \
                      .filter({"name": "KingHamlet"}).nth(0)["id"].run(conn)
    prince_hamlet_id = r.db("rethinkdb").table("server_config") \
                        .filter({"name": "PrinceHamlet"}).nth(0)["id"].run(conn)

    print("Creating two tables...")
    r.db_create("test").run(conn)
    res = r.db("rethinkdb").table("table_config").insert([
        # The `test` table will remain readable when `KingHamlet` is removed.
        {
            "db": "test",
            "name": "test",
            "primary_key": "id",
            "shards": [{
                "director": "PrinceHamlet",
                "replicas": ["PrinceHamlet", "KingHamlet"]
                }],
            "write_acks": "single"
        },
        # The `test2` table will raise a `table_needs_primary` issue
        {
            "db": "test",
            "name": "test2",
            "primary_key": "id",
            "shards": [{
                "director": "KingHamlet",
                "replicas": ["PrinceHamlet", "KingHamlet"]
                }],
            "write_acks": "single"
        },
        # The `test3` table will raise a `data_lost` issue
        {
            "db": "test",
            "name": "test3",
            "primary_key": "id",
            "shards": [{
                "director": "KingHamlet",
                "replicas": ["KingHamlet"]
                }],
            "write_acks": "single"
        }
        ]).run(conn)
    assert res["inserted"] == 3, res
    r.table_wait("test", "test2", "test3").run(conn)
    res = r.table("test").insert([{}]*100).run(conn)
    assert res["inserted"] == 100
    res = r.table("test2").insert([{}]*100).run(conn)
    assert res["inserted"] == 100
    res = r.table("test3").insert([{}]*100).run(conn)
    assert res["inserted"] == 100

    print("Killing one of them...")
    king_hamlet.close()
    time.sleep(1)
    cluster.check()

    print("Checking that the other has an issue...")
    issues = list(r.db("rethinkdb").table("issues").run(conn))
    pprint.pprint(issues)
    assert len(issues) == 1, issues
    assert issues[0]["type"] == "server_down"
    assert issues[0]["critical"]
    assert "KingHamlet" in issues[0]["description"]
    assert issues[0]["info"]["server"] == "KingHamlet"
    assert issues[0]["info"]["server_id"] == king_hamlet_id
    assert issues[0]["info"]["affected_servers"] == ["PrinceHamlet"]
    assert issues[0]["info"]["affected_server_ids"] == [prince_hamlet_id]

    test_status, test2_status, test3_status = \
        r.table_status("test", "test2", "test3").run(conn)
    assert test_status["status"]["ready_for_writes"], test_status
    assert not test_status["status"]["all_replicas_ready"], test_status
    assert test2_status["status"]["ready_for_outdated_reads"], test2_status
    assert not test2_status["status"]["ready_for_reads"], test2_status
    assert not test3_status["status"]["ready_for_outdated_reads"], test3_status

    print("Permanently removing the dead one...")
    res = r.db("rethinkdb").table("server_config").filter({"name": "KingHamlet"}) \
           .delete().run(conn)
    assert res["deleted"] == 1
    assert res["errors"] == 0

    print("Checking the issues that were generated...")
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

    test_status, test2_status, test3_status = \
        r.table_status("test", "test2", "test3").run(conn)
    assert test_status["status"]["all_replicas_ready"]
    assert test2_status["status"]["ready_for_outdated_reads"]
    assert not test2_status["status"]["ready_for_reads"]
    assert not test3_status["status"]["ready_for_outdated_reads"]
    assert r.table_config("test").nth(0)["shards"].run(conn) == [{
        "director": "PrinceHamlet",
        "replicas": ["PrinceHamlet"]
        }]
    assert r.table_config("test2").nth(0)["shards"].run(conn) == [{
        "director": None,
        "replicas": ["PrinceHamlet"]
        }]
    assert r.table_config("test3").nth(0)["shards"].run(conn) == [{
        "director": None,
        "replicas": []
        }]

    print("Testing that having director=None doesn't break `table_config`...")
    # By changing the table's name, we force a write to `table_config`, which tests the
    # code path that writes `"director": None`.
    res = r.table_config("test2").update({"name": "test2x"}).run(conn)
    assert res["errors"] == 0
    res = r.table_config("test2x").update({"name": "test2"}).run(conn)
    assert res["errors"] == 0
    assert r.table_config("test2").nth(0)["shards"].run(conn) == [{
        "director": None,
        "replicas": ["PrinceHamlet"]
        }]

    print("Fixing table `test2`...")
    r.table("test2").reconfigure(shards=1, replicas=1).run(conn)
    r.table_wait("test2").run(conn)

    print("Fixing table `test3`...")
    r.table("test3").reconfigure(shards=1, replicas=1).run(conn)
    r.table_wait("test3").run(conn)

    print("Bringing the dead server back as a ghost...")
    ghost_of_king_hamlet = driver.Process(cluster, king_hamlet_files,
        console_output = "king-hamlet-ghost-log",
        executable_path = executable_path, command_prefix = command_prefix)
    ghost_of_king_hamlet.wait_until_started_up()
    cluster.check()

    print("Checking that there is an issue...")
    issues = list(r.db("rethinkdb").table("issues").run(conn))
    pprint.pprint(issues)
    assert len(issues) == 1, issues
    assert issues[0]["type"] == "server_ghost"
    assert not issues[0]["critical"]
    assert issues[0]["info"]["server_id"] == king_hamlet_id
    assert issues[0]["info"]["hostname"] == socket.gethostname()
    assert issues[0]["info"]["pid"] == ghost_of_king_hamlet.process.pid

    print("Checking table contents...")
    assert r.table("test").count().run(conn) == 100
    assert r.table("test2").count().run(conn) == 100
    assert r.table("test3").count().run(conn) == 0

    cluster.check_and_stop()
print "Done."
