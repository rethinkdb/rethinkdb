#!/usr/bin/env python
# Copyright 2014 RethinkDB, all rights reserved.

"""The `interface.table_acks_durability` test checks that write-acks and durability settings on tables behave as expected."""

from __future__ import print_function

import os, pprint, sys, time

try:
    xrange
except NameError:
    xrange = range

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

r = utils.import_python_driver()
dbName, _ = utils.get_test_db_table()

num_live = num_dead = 3

print("Starting cluster of %d servers (%.2fs)" % (num_live + num_dead, time.time() - startTime))
with driver.Cluster(output_folder='.') as cluster:

    # The "live" processes will remain alive for the entire test. The "dead" processes
    # will be killed after we create some tables.
    
    live_names = ["l%d" % (i+1) for i in xrange(num_live)]
    dead_names = ["d%d" % (i+1) for i in xrange(num_dead)]
    
    for name in live_names:
        driver.Process(cluster=cluster, files=name, command_prefix=command_prefix, extra_options=serve_options)
    
    server = cluster[0]
    
    dead_procs = []
    for name in dead_names:
        dead_procs.append(driver.Process(cluster=cluster, files=name, command_prefix=command_prefix, extra_options=serve_options))
    
    cluster.wait_until_ready()
    cluster.check()
    
    print("Establishing ReQl connections (%.2fs)" % (time.time() - startTime))
    
    conn = r.connect(host=server.host, port=server.driver_port)

    def mks(nl, nd, primary_replica="l"):
        """Helper function for constructing entries for `table_config.shards`. Returns a
        shard with "nl" live replicas and "nd" dead ones. The value of "primary_replica"
        determines if the primary replica will be a live one or dead one."""
        assert nl <= num_live and nd <= num_dead
        assert (primary_replica == "l" and nl > 0) or (primary_replica == "d" and nd > 0)
        replicas = live_names[:nl] + dead_names[:nd]
        return {"replicas": replicas, "primary_replica": "%s1" % primary_replica}
    def mkr(nl, nd, mode):
        """Helper function for constructing lists for `table_config.write_acks`. Returns
        an ack requirement with "nl" live replicas and "nd" dead ones, and the given mode
        for "single" or "majority"."""
        assert nl <= num_live and nd <= num_dead
        replicas = live_names[:nl] + dead_names[:nd]
        return {"replicas": replicas, "acks": mode}

    # Each test is a tuple with four parts:
    #  1. A list of shards, in the same format as `table_config.shards`.
    #  2. A value in the same format as `table_config.write_acks`.
    #  3. The level of availability we expect for the table after the "dead" servers have
    #     shut down, expressed as a string with the chars `a`, `w`, `r`, and/or `o`,
    #     which correspond to the four fields in the `status` group of `table_status`.
    #  4. The level of availability we expect after the "dead" servers have been
    #     permanently removed and the primary replica (if dead) reassigned to a live server.
    tests = [
        ([mks(3, 0)], "single", "awro", "awro"),
        ([mks(0, 3, "d")], "single", "", ""),
        ([mks(1, 0)], "majority", "awro", "awro"),
        ([mks(0, 1, "d")], "majority", "", ""),
        ([mks(2, 1)], "single", "wro", "awro"),
        ([mks(1, 2)], "single", "wro", "awro"),
        ([mks(2, 1)], "majority", "wro", "awro"),
        ([mks(2, 1, "d")], "majority", "o", "o"),
        ([mks(1, 2)], "majority", "ro", "awro"),
        ([mks(2, 2)], "majority", "ro", "awro"),
        ([mks(2, 2, "d")], "majority", "o", "o"),
        ([mks(2, 2)], [mkr(1, 1, "single")], "wro", "awro"),
        ([mks(2, 2)], [mkr(1, 1, "single"), mkr(2, 0, "majority")], "wro", "awro"),
        ([mks(3, 3)], [mkr(3, 0, "majority"), mkr(0, 3, "majority")], "ro", "ro"),
        ([mks(3, 3)], [mkr(3, 0, "majority"), mkr(0, 1, "single")], "ro", "ro"),
        ([mks(2, 2), mks(2, 2, "d")], [mkr(2, 0, "majority"), mkr(0, 2, "majority")],
            "o", "o"),
        ]

    print("Creating tables for tests (%.2fs)" % (time.time() - startTime))
    r.db_create("test").run(conn)
    for i, (shards, write_acks, readiness_1, readiness_2) in enumerate(tests):
        conf = {"shards": shards, "write_acks": write_acks}
        t = "table%d" % (i+1)
        print(t, conf)
        res = r.db(dbName).table_create(t).run(conn)
        assert res["tables_created"] == 1, res
        res = r.db(dbName).table(t).config().update(conf).run(conn)
        assert res["errors"] == 0, res
        r.db(dbName).table(t).wait().run(conn)
        res = r.db(dbName).table(t).insert([{}]*1000).run(conn)
        assert res["errors"] == 0 and res["inserted"] == 1000, res

    issues = list(r.db("rethinkdb").table("current_issues").run(conn))
    assert not issues, repr(issues)

    print("Killing the designated 'dead' servers (%.2fs)" % (time.time() - startTime))
    for proc in dead_procs:
        proc.check_and_stop()

    print("Checking table statuses (%.2fs)" % (time.time() - startTime))
    def check_table_status(name, expected_readiness):
        tested_readiness = ""
        try:
            res = r.db(dbName).table(name).update({"x": r.row["x"].default(0).add(1)}).run(conn)
        except r.RqlRuntimeError, e:
            # This can happen if we aren't available for reading either
            pass
        else:
            assert res["errors"] == 0 and res["replaced"] == 1000
            tested_readiness += "w"
        try:
            res = r.db(dbName).table(name).count().run(conn)
        except r.RqlRuntimeError, e:
            pass
        else:
            assert res == 1000
            tested_readiness += "r"
        try:
            res = r.db(dbName).table(name).count().run(conn, read_mode="outdated")
        except r.RqlRuntimeError, e:
            pass
        else:
            assert res == 1000
            tested_readiness += "o"
        res = r.db(dbName).table(name).status().run(conn)
        reported_readiness = ""
        if res["status"]["all_replicas_ready"]:
            reported_readiness += "a"
        if res["status"]["ready_for_writes"]:
            reported_readiness += "w"
        if res["status"]["ready_for_reads"]:
            reported_readiness += "r"
        if res["status"]["ready_for_outdated_reads"]:
            reported_readiness += "o"
        print("%s expect=%r test=%r report=%r" % (name, expected_readiness, tested_readiness, reported_readiness))
        assert expected_readiness.replace("a", "") == tested_readiness
        assert expected_readiness == reported_readiness

    for i, (shards, write_acks, readiness_1, readiness_2) in enumerate(tests):
        check_table_status("table%d" % (i+1), readiness_1)

    print("Permanently removing the designated 'dead' servers (%.2fs)" % (time.time() - startTime))
    res = r.db("rethinkdb").table("server_config").filter(lambda s: r.expr(dead_names).contains(s["name"])).delete().run(conn)
    assert res["deleted"] == num_dead, res

    print("Checking table statuses (%.2fs)" % (time.time() - startTime))
    for i, (shards, write_acks, readiness_1, readiness_2) in enumerate(tests):
        check_table_status("table%d" % (i+1), readiness_2)

    print("Checking for issues (%.2fs)" % (time.time() - startTime))
    issues = list(r.db("rethinkdb").table("current_issues").run(conn))
    pprint.pprint(issues)
    issues_by_table = {}
    for issue in issues:
        assert issue["type"] in ["write_acks", "data_lost", "table_needs_primary"]
        issues_by_table[issue["info"]["table"]] = issue["type"]
    for i, (shards, write_acks, readiness_1, readiness_2) in enumerate(tests):
        t = "table%d" % (i+1)
        if readiness_2 == "":
            assert issues_by_table[t] == "data_lost"
        elif readiness_2 == "o":
            assert issues_by_table[t] == "table_needs_primary"
        elif readiness_2 == "ro":
            assert issues_by_table[t] == "write_acks"
        elif readiness_2 == "wro":
            assert False, "This is impossible since we removed the dead servers"
        elif readiness_2 == "awro":
            assert t not in issues_by_table

    print("Running auxiliary tests (%.2fs)" % (time.time() - startTime))
    res = r.db(dbName).table_create("aux").run(conn)
    assert res["tables_created"] == 1, res
    res = r.db(dbName).table("aux").config() \
        .update({"shards": [{"primary_replica": "l1", "replicas": ["l1"]}]}).run(conn)
    assert res["errors"] == 0, res
    r.db(dbName).table("aux").wait().run(conn)
    def test_ok(change):
        print(repr(change))
        res = r.db(dbName).table("aux").config().update(change).run(conn)
        assert res["errors"] == 0 and res["replaced"] == 1, res
        print("OK")
    def test_fail(change):
        print(repr(change))
        res = r.db(dbName).table("aux").config().update(change).run(conn)
        assert res["errors"] == 1 and res["replaced"] == 0, res
        print("Failed (as expected):", repr(res["first_error"]))
    test_ok({"durability": "soft"})
    test_ok({"durability": "hard"})
    test_fail({"durability": "the consistency of toothpaste"})
    test_ok({"write_acks": [{"replicas": ["l1"], "acks": "single"}]})
    test_fail({"write_acks": "this is a string"})
    test_fail({"write_acks": [{"replicas": ["not_a_server"], "acks": "single"}]})
    test_fail({"write_acks": [{"replicas": 1, "acks": "single"}]})
    test_fail({"write_acks": [{"replicas": ["l1"], "acks": 12345}]})
    test_fail({"write_acks": [{"replicas": ["l1"]}]})
    test_fail({"write_acks": [{"acks": "single"}]})
    test_fail({"write_acks": [{"replicas": ["l1"], "acks": "single", "foo": "bar"}]})

    print("Checking that unsatisfiable write acks are rejected (%.2fs)" % (time.time() - startTime))
    test_ok({
        "shards": [
            {"primary_replica": "l1", "replicas": ["l1", "l2", "l3"]},
            {"primary_replica": "l1", "replicas": ["l1"]},
            ],
        "write_acks": "single"
        })
    test_fail({"write_acks": "majority"})

    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))

