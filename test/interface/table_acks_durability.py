#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.
import sys, os, time, traceback
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils
from vcoptparse import *
r = utils.import_python_driver()

"""The `interface.table_acks_durability` test checks that write-acks and durability
settings on tables behave as expected."""

op = OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = \
        scenario_common.parse_mode_flags(opts)

    # The "live" process will remain alive for the entire test. The "dead" processes will
    # be killed after we create some tables
    num_live = num_dead = 3
    print "Spinning up %d processes..." % (num_live + num_dead)
    def make_procs(names):
        files, procs = [], []
        for name in names:
            files.append(driver.Files(
                metacluster,
                log_path = "create-output-%s" % name,
                server_name = name,
                executable_path = executable_path,
                command_prefix = command_prefix))
            procs.append(driver.Process(
                cluster,
                files[-1],
                log_path = "serve-output-%s" % name,
                executable_path = executable_path,
                command_prefix = command_prefix,
                extra_options = serve_options))
        return files, procs
    live_names = ["l%d" % (i+1) for i in xrange(num_live)]
    dead_names = ["d%d" % (i+1) for i in xrange(num_dead)]
    live_files, live_procs = make_procs(live_names)
    dead_files, dead_procs = make_procs(dead_names)

    for p in live_procs + dead_procs:
        p.wait_until_started_up()
    cluster.check()

    conn = r.connect("localhost", live_procs[0].driver_port)

    def mks(nl, nd, primary = "l"):
        """Helper function for constructing entries for `table_config.shards`. Returns a
        shard with "nl" live replicas and "nd" dead ones. The value of "primary"
        determines if the primary replica will be a live one or dead one."""
        assert nl <= num_live and nd <= num_dead
        assert (primary == "l" and nl > 0) or (primary == "d" and nd > 0)
        replicas = live_names[:nl] + dead_names[:nd]
        return {"replicas": replicas, "director": "%s1" % primary}
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
    #     which correspond to the four `ready_*` fields in `table_status`.
    #  4. The level of availability we expect after the "dead" servers have been
    #     permanently removed and the primary (if dead) reassigned to a live server.
    tests = [
        ([mks(3, 0)], "single", "awro", "awro"),
        ([mks(0, 3, "d")], "single", "", ""),
        ([mks(1, 0)], "majority", "awro", "awro"),
        ([mks(0, 1, "d")], "majority", "", ""),
        ([mks(2, 1)], "single", "wro", "awro"),
        ([mks(1, 2)], "single", "wro", "awro"),
        ([mks(2, 1)], "majority", "wro", "awro"),
        ([mks(2, 1, "d")], "majority", "o", "awro"),
        ([mks(1, 2)], "majority", "ro", "awro"),
        ([mks(2, 2)], "majority", "ro", "awro"),
        ([mks(2, 2, "d")], "majority", "o", "awro"),
        ([mks(2, 2)], [mkr(1, 1, "single")], "wro", "awro"),
        ([mks(2, 2)], [mkr(1, 1, "single"), mkr(2, 0, "majority")], "wro", "awro"),
        ([mks(3, 3)], [mkr(3, 0, "majority"), mkr(0, 3, "majority")], "ro", "ro"),
        ([mks(3, 3)], [mkr(3, 0, "majority"), mkr(0, 1, "single")], "ro", "ro"),
        ([mks(2, 2), mks(2, 2, "d")], [mkr(2, 0, "majority"), mkr(0, 2, "majority")],
            "o", "awro"),
        ]

    print "Creating tables for tests..."
    r.db_create("test").run(conn)
    for i, (shards, write_acks, readiness_1, readiness_2) in enumerate(tests):
        conf = {"shards": shards, "write_acks": write_acks}
        print "%d/%d" % (i+1, len(tests)), conf
        t = "table%d" % (i+1)
        res = r.table_create(t).run(conn)
        assert res["created"] == 1, res
        res = r.table_config(t).update(conf).run(conn)
        assert res["errors"] == 0, res
        r.table_wait(t).run(conn)
        res = r.table(t).insert([{}]*1000).run(conn)
        assert res["errors"] == 0 and res["inserted"] == 1000, res

    print "Killing the designated 'dead' servers..."
    for proc in dead_procs:
        proc.check_and_stop()

    print "Checking table statuses..."
    def check_table_status(name, expected_readiness):
        tested_readiness = ""
        try:
            res = r.table(name).update({"x": r.row["x"].default(0).add(1)}).run(conn)
        except r.RqlRuntimeError, e:
            # This can happen if we aren't available for reading either
            pass
        else:
            assert res["errors"] == 0 and res["replaced"] == 1000
            tested_readiness += "w"
        try:
            res = r.table(name).count().run(conn)
        except r.RqlRuntimeError, e:
            pass
        else:
            assert res == 1000
            tested_readiness += "r"
        try:
            res = r.table(name).count().run(conn, use_outdated = True)
        except r.RqlRuntimeError, e:
            pass
        else:
            assert res == 1000
            tested_readiness += "o"
        res = r.table_status(name).nth(0).run(conn)
        reported_readiness = ""
        if res["ready_completely"]:
            reported_readiness += "a"
        if res["ready_for_writes"]:
            reported_readiness += "w"
        if res["ready_for_reads"]:
            reported_readiness += "r"
        if res["ready_for_outdated_reads"]:
            reported_readiness += "o"
        print "%s expect=%r test=%r report=%r" % \
            (name, expected_readiness, tested_readiness, reported_readiness)
        assert expected_readiness.replace("a", "") == tested_readiness
        assert expected_readiness == reported_readiness

    for i, (shards, write_acks, readiness_1, readiness_2) in enumerate(tests):
        check_table_status("table%d" % (i+1), readiness_1)

    print "Permanently removing the designated 'dead' servers..."
    res = r.db("rethinkdb").table("server_config") \
           .filter(lambda s: r.expr(dead_names).contains(s["name"])).delete().run(conn)
    assert res["deleted"] == num_dead, res
    res = r.table_config().update(lambda conf: {
        "shards": conf["shards"].map(lambda shard: {
            "replicas": shard["replicas"],
            "director": shard["director"].default(shard["replicas"].nth(0)).default(None)
        })}).run(conn)
    assert res["errors"] == 0, res

    print "Checking table statuses..."
    for i, (shards, write_acks, readiness_1, readiness_2) in enumerate(tests):
        check_table_status("table%d" % (i+1), readiness_2)
    # TODO: Check that issues exist for write acks that are now unsatisfiable

    print "Running auxiliary tests..."
    res = r.table_create("aux").run(conn)
    assert res["created"] == 1, res
    def test_ok(change):
        print repr(change)
        res = r.table_config("aux").update(change).run(conn)
        assert res["errors"] == 0 and res["replaced"] == 1
        print "OK"
    def test_fail(change):
        print repr(change)
        res = r.table_config("aux").update(change).run(conn)
        assert res["errors"] == 1 and res["replaced"] == 0
        print "Failed (as expected):", repr(res["first_error"])
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
    # TODO: Test putting up unsatisfiable write acks

    cluster.check_and_stop()
print "Done."

