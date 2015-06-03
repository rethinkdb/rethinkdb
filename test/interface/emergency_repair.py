#!/usr/bin/env python
# Copyright 2010-2015 RethinkDB, all rights reserved.

"""The `interface.emergency_repair` test checks that the `emergency_repair` mode of
`table.reconfigure()` works properly."""

from __future__ import print_function
import time, os, sys, pprint, pdb

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

r = utils.import_python_driver()

# We start two servers: "a" and "x". Then we create a bunch of tables that are configured
# across "a" and "x" in various ways. Then we kill "x" and repair the various tables.

print("Starting cluster of 2 servers (%.2fs)" % (time.time() - startTime))
with driver.Cluster(initial_servers=['a', 'x'], output_folder='.',
        command_prefix=command_prefix, extra_options=serve_options) as cluster:
    cluster.check()

    conn = r.connect("localhost", cluster[0].driver_port)

    res = r.db_create("test").run(conn)
    assert res["dbs_created"] == 1

    docs_per_table = 100
    def make_table(name, shards):
        print("Preparing table '%s' (%.2fs)" % (name, time.time() - startTime))
        res = r.db("rethinkdb").table("table_config").insert({
            "name": name,
            "db": "test",
            "shards": shards
            }).run(conn)
        assert res.get("inserted") == 1, res
        res = r.table(name).wait(wait_for="all_replicas_ready").run(conn)
        assert res.get("ready") == 1, res
        res = r.table(name).insert({"number": i} for i in xrange(docs_per_table)).run(conn)
        assert res.get("inserted") == 100

    make_table("no_repair_1", [
        {"primary_replica": "a", "replicas": ["a"]}])
    make_table("no_repair_2", [
        {"primary_replica": "a", "replicas": ["a", "x"], "nonvoting_replicas": ["x"]}])
    make_table("rollback", [
        {"primary_replica": "a", "replicas": ["a", "x"]}])
    make_table("rollback_nonvoting", [
        {"primary_replica": "x", "replicas": ["a", "x"], "nonvoting_replicas": ["a"]}])
    make_table("erase", [
        {"primary_replica": "a", "replicas": ["a"]},
        {"primary_replica": "x", "replicas": ["x"]}])
    make_table("rollback_then_erase", [
        {"primary_replica": "a", "replicas": ["a", "x"]},
        {"primary_replica": "x", "replicas": ["x"]}])
    make_table("total_loss", [
        {"primary_replica": "x", "replicas": ["x"]}])

    print("Killing server 'x' (%.2fs)" % (time.time() - startTime))
    cluster[1].kill()

    def eq_config(l_shards, r_shards):
        if len(l_shards) != len(r_shards): return False
        for (l, r) in zip(l_shards, r_shards):
            if l["primary_replica"] != r["primary_replica"]: return False
            if set(l["replicas"]) != set(r["replicas"]): return False
            if set(l.get("nonvoting_replicas", [])) != \
                set(r.get("nonvoting_replicas", [])): return False
        return True

    def repair(name, repair_type, expect):
        print("Repairing table '%s' with mode '%s' (%.2fs)" % (name, repair_type, time.time() - startTime))
        res = r.table(name).reconfigure(emergency_repair=repair_type, dry_run=True).run(conn)
        print("New config, as expected:", res["config_changes"][0]["new_val"]["shards"])
        assert res.get("repaired") == 0, res
        assert eq_config(res["config_changes"][0]["new_val"]["shards"], expect), \
            "res=%s, expect=%s" % (res, expect)
        old_config = r.table(name).config().run(conn)
        assert eq_config(res["config_changes"][0]["old_val"]["shards"], old_config["shards"]), \
            "res=%s, old_config=%s" % (res, old_config)
        res = r.table(name).reconfigure(emergency_repair=repair_type).run(conn)
        assert res.get("repaired") == 1, res
        assert eq_config(res["config_changes"][0]["new_val"]["shards"], expect), \
            "res=%s, expect=%s" % (res, expect)
        new_config = r.table(name).config().run(conn)
        assert eq_config(new_config["shards"], expect), \
            "new_config=%s, expect=%s" % (new_config, expect)

    def bad_repair(name, repair_type, msg):
        print("Repairing table '%s' with mode '%s' (this should fail) (%.2fs)" %
            (name, repair_type, time.time() - startTime))
        try:
            r.table(name).reconfigure(emergency_repair=repair_type).run(conn)
        except r.RqlRuntimeError, e:
            print("As expected, it failed:", repr(str(e).split(" in:")[0]))
            assert msg in str(e), e
        else:
            raise ValueError("Expected emergency repair to fail")

    def wait_for_table(name):
        try:
            res = r.table(name).wait(wait_for="ready_for_writes", timeout=10).run(conn)
            assert res["ready"] == 1
        except r.RqlRuntimeError, e:
            pprint.pprint(r.table(name).status().run(conn))
            pdb.set_trace()
            raise

    def check_table(name):
        print("Checking contents of table '%s' (%.2fs)" % (name, time.time() - startTime))
        wait_for_table(name)
        assert r.table(name).count().run(conn) == docs_per_table

    def check_table_half(name):
        print("Checking contents of table '%s' (expect half erased) (%.2fs)" % (name, time.time() - startTime))
        wait_for_table(name)
        count = r.table(name).count().run(conn)
        assert 0.25*docs_per_table < count < 0.75*docs_per_table, \
            ("Found %d rows, expected about %d" % (count, 0.50*docs_per_table))

    check_table("no_repair_1")
    bad_repair("no_repair_1", "unsafe_rollback",
        "This table doesn't need to be repaired.")

    check_table("no_repair_2")
    bad_repair("no_repair_2", "unsafe_rollback",
        "This table doesn't need to be repaired.")

    repair("rollback", "unsafe_rollback",
        [{"primary_replica": "a", "replicas": ["a", "x"], "nonvoting_replicas": ["x"]}])
    check_table("rollback")

    repair("rollback_nonvoting", "unsafe_rollback",
        [{"primary_replica": "a", "replicas": ["a", "x"], "nonvoting_replicas": ["x"]}])
    check_table("rollback_nonvoting")

    bad_repair("erase", "unsafe_rollback",
        "One or more shards of this table have no available replicas.")
    repair("erase", "unsafe_rollback_or_erase", [
        {"primary_replica": "a", "replicas": ["a"]}])
    check_table_half("erase")

    repair("rollback_then_erase", "unsafe_rollback", [
        {"primary_replica": "a", "replicas": ["a", "x"], "nonvoting_replicas": ["x"]},
        {"primary_replica": "x", "replicas": ["x"]}])
    bad_repair("rollback_then_erase", "unsafe_rollback",
        "One or more shards of this table have no available replicas.")
    repair("rollback_then_erase", "unsafe_rollback_or_erase", [
        {"primary_replica": "a", "replicas": ["a", "x"], "nonvoting_replicas": ["x"]},
        {"primary_replica": "a", "replicas": ["a"]}])
    check_table_half("rollback_then_erase")

    bad_repair("total_loss", "unsafe_rollback_or_erase",
        "At least one of a table's replicas must be accessible in order to repair it.")
    res = r.table_drop("total_loss").run(conn)
    assert res["tables_dropped"] == 1

    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))

