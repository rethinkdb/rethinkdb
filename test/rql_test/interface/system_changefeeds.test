#!/usr/bin/env python
# Copyright 2014-2016 RethinkDB, all rights reserved.

"""Check that changefeeds on system tables correctly notify when changes occur."""

import os, sys, time, traceback

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

r = utils.import_python_driver()

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, server_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

with driver.Cluster(output_folder='./servers', command_prefix=command_prefix, extra_options=server_options) as cluster:
    proc1 = driver.Process(cluster=cluster, name='a', server_tags='a_tag', console_output=True, command_prefix=command_prefix, extra_options=server_options)
    proc2 = driver.Process(cluster=cluster, name='b', server_tags='b_tag', console_output=True, command_prefix=command_prefix, extra_options=server_options)
    conn = r.connect(proc1.host, proc1.driver_port)
    
    # wait for all of the startup log messages to be past
    logMonitor =  r.db('rethinkdb').table('logs').changes().run(conn)
    while True:
        try:
            logMonitor.next(wait=2)
        except r.ReqlTimeoutError:
            break
    
    feeds = { }
    for name in ["cluster_config", "db_config", "current_issues", "logs", "permissions", "server_config", "server_status", "table_config", "table_status", "users"]:
        feeds[name] = r.db('rethinkdb').table(name).changes().run(conn)
    
    def check(expected, timer):
        time.sleep(timer)
        for name, feed in feeds.items():
            changes = []
            try:
                while True:
                    change = feed.next(wait=False)
                    if 'old_val' in change:
                        changes.append(change)
            except r.ReqlTimeoutError:
                pass
            except r.Error as e:
                utils.print_with_time('Exception on feed "%s"' % name)
                traceback.print_exception()
                sys.exit(1)
            if name in expected:
                assert len(changes) > 0, "Expected changes on %s, found none." % name
            else:
                assert len(changes) == 0, "Expected no changes on %s, found:\n%s" % (name, utils.pformat)
    check([], 5.0)

    utils.print_with_time("Creating database...")
    res = r.db_create("test").run(conn)
    assert res.get("dbs_created", 0) == 1, res
    check(["db_config"], 1.0)

    utils.print_with_time("Creating tables...")
    res = r.table_create("test", replicas={"a_tag": 1}, primary_replica_tag="a_tag").run(conn)
    assert res["tables_created"] == 1, res
    res = r.table_create("test2", replicas={"b_tag": 1}, primary_replica_tag="b_tag").run(conn)
    assert res["tables_created"] == 1, res
    check(["table_config", "table_status", "logs"], 1.5)
    
    utils.print_with_time("Creating feeds...")
    feeds["test_config"]  = r.table('test').config().changes().run(conn)
    feeds["test_status"]  = r.table('test').status().changes().run(conn)
    feeds["test2_config"] = r.table('test2').config().changes().run(conn)
    feeds["test2_status"] = r.table('test2').status().changes().run(conn)
    
    utils.print_with_time("Adding replicas...")
    res = r.table("test").config().update({"shards": [{"primary_replica": "a", "replicas": ["a", "b"]}]}).run(conn)
    assert res["errors"] == 0, res
    r.table("test").wait(wait_for="all_replicas_ready").run(conn)
    check(["table_config", "table_status", "test_config", "test_status", "logs"], 1.5)

    utils.print_with_time("Renaming server...")
    res = r.db("rethinkdb").table("server_config").filter({"name": "b"}).update({"name": "c"}).run(conn)
    assert res["replaced"] == 1 and res["errors"] == 0, res
    check(["logs", "server_config", "server_status", "table_config", "table_status",
        "test_config", "test_status", "test2_config", "test2_status"], 1.5)

    utils.print_with_time("Killing one server...")
    proc2.check_and_stop()
    check(["logs", "server_config", "server_status", "table_config", "table_status", "current_issues",
        "test_status", "test2_config", "test2_status"], 1.5)

    utils.print_with_time("Shutting everything down...")
utils.print_with_time("Done.")
