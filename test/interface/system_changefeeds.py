#!/usr/bin/env python
# Copyright 2014 RethinkDB, all rights reserved.

from __future__ import print_function

import os, pprint, sys, time, threading, traceback

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

r = utils.import_python_driver()

"""The `interface.system_changefeeds` test checks that changefeeds on system tables
correctly notify when changes occur."""

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

class AsyncChangefeed(object):
    def __init__(self, host, port, query):
        self.conn = r.connect(host, port)
        self.stopping = False
        self.err = None
        self.changes = []
        self.thr = threading.Thread(target = self.run, args = (query, ))
        self.thr.daemon = True
        self.thr.start()
        time.sleep(0.5)
    def run(self, query):
        try:
            for x in eval(query).changes().run(self.conn):
                # Throw away initial values
                if "old_val" in x:
                    self.changes.append(x)
        except Exception, e:
            self.err = sys.exc_info()
    def check(self):
        if self.err is not None:
            print("Exception from other thread:")
            traceback.print_exception(*self.err)
            sys.exit(1)

with driver.Metacluster() as metacluster:
    cluster1 = driver.Cluster(metacluster)
    _, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    
    print("Spinning up two processes...")
    files1 = driver.Files(metacluster, console_output="create-output-1", server_name="a", server_tags=["a_tag"], command_prefix=command_prefix)
    proc1 = driver.Process(cluster1, files1, console_output="serve-output-1", command_prefix=command_prefix, extra_options=serve_options)
    files2 = driver.Files(metacluster, console_output="create-output-2", server_name="b", server_tags=["b_tag"], command_prefix=command_prefix)
    proc2 = driver.Process(cluster1, files2, console_output="serve-output-2", command_prefix=command_prefix, extra_options=serve_options)
    proc1.wait_until_started_up()
    proc2.wait_until_started_up()
    cluster1.check()

    # This is necessary because a few log messages may be printed even after
    # `wait_until_started_up()` returns.
    time.sleep(5.0)

    conn = r.connect(proc1.host, proc1.driver_port)
    tables = ["cluster_config", "db_config", "current_issues", "logs", "server_config",
        "server_status", "table_config", "table_status"]
    feeds = { }
    for name in tables:
        feeds[name] = AsyncChangefeed(proc1.host, proc1.driver_port,
            "r.db('rethinkdb').table(%r)" % name)

    def check(expected, timer):
        time.sleep(timer)
        for name, feed in feeds.iteritems():
            feed.check()
            if name in expected:
                assert len(feed.changes) > 0, \
                    "Expected changes on %s, found none." % name
                feed.changes = []
            else:
                assert len(feed.changes) == 0, \
                    "Expected no changes on %s, found %s." % (name, feed.changes)
    check([], 5.0)

    print("Changing auth key...")
    res = r.db("rethinkdb").table("cluster_config").get("auth") \
           .update({"auth_key": "foo"}).run(conn)
    assert res["replaced"] == 1 and res["errors"] == 0, res
    res = r.db("rethinkdb").table("cluster_config").get("auth") \
           .update({"auth_key": None}).run(conn)
    check(["cluster_config"], 1.0)

    print("Creating database...")
    res = r.db_create("test").run(conn)
    assert res.get("dbs_created", 0) == 1, res
    check(["db_config"], 1.0)

    print("Creating tables...")
    res = r.table_create("test", replicas={"a_tag": 1}, primary_replica_tag="a_tag").run(conn)
    assert res["tables_created"] == 1, res
    res = r.table_create("test2", replicas={"b_tag": 1}, primary_replica_tag="b_tag").run(conn)
    assert res["tables_created"] == 1, res
    check(["table_config", "table_status", "logs"], 1.0)

    feeds["test_config"] = AsyncChangefeed(proc1.host, proc1.driver_port,
        "r.table('test').config()")
    feeds["test_status"] = AsyncChangefeed(proc1.host, proc1.driver_port,
        "r.table('test').status()")
    feeds["test2_config"] = AsyncChangefeed(proc1.host, proc1.driver_port,
        "r.table('test2').config()")
    feeds["test2_status"] = AsyncChangefeed(proc1.host, proc1.driver_port,
        "r.table('test2').status()")

    res = r.table("test").config() \
           .update({"shards": [{"primary_replica": "a", "replicas": ["a", "b"]}]}).run(conn)
    assert res["errors"] == 0, res
    r.table("test").wait().run(conn)
    check(["table_config", "table_status", "test_config", "test_status"], 1.0)

    print("Renaming server...")
    res = r.db("rethinkdb").table("server_config").filter({"name": "b"}) \
           .update({"name": "c"}).run(conn)
    assert res["replaced"] == 1 and res["errors"] == 0, res
    check(["logs", "server_config", "server_status", "table_config", "table_status",
        "test_config", "test_status", "test2_config", "test2_status"], 1.0)

    print("Killing one server...")
    proc2.check_and_stop()
    check(["logs", "server_status", "table_status", "current_issues",
        "test_status", "test2_status"], 1.0)

    print("Declaring it dead...")
    res = r.db("rethinkdb").table("server_config").filter({"name": "c"}).delete() \
           .run(conn)
    assert res["deleted"] == 1 and res["errors"] == 0, res
    check(["server_config", "server_status", "table_config", "table_status", "current_issues",
        "test_config", "test_status", "test2_config", "test2_status"], 1.0)

    print("Shutting everything down...")
    cluster1.check_and_stop()
print("Done.")
