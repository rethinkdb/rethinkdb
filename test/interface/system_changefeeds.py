#!/usr/bin/env python
# Copyright 2014-2015 RethinkDB, all rights reserved.

"""Check that changefeeds on system tablescorrectly notify when changes occur."""

import os, pprint, sys, time, threading, traceback

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

r = utils.import_python_driver()

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, server_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

class AsyncChangefeed(threading.Thread):
    daemon = True
    
    conn = None
    query = None
    
    err = None
    changes = None
    
    def __init__(self, server, query):
        super(AsyncChangefeed, self).__init__()
        self.conn = r.connect(server.host, server.driver_port)
        self.changes = []
        self.query = query
        self.start()
        time.sleep(0.5)
    
    def run(self):
        try:
            for x in self.query.changes().run(self.conn):
                # Throw away initial values
                if "old_val" in x:
                    self.changes.append(x)
        except Exception, e:
            self.err = sys.exc_info()
    
    def check(self):
        if self.err is not None:
            utils.print_with_time("Exception from other thread:")
            traceback.print_exception(*self.err)
            sys.exit(1)

with driver.Cluster(output_folder='.', ) as cluster:
    proc1 = driver.Process(cluster=cluster, name='a', server_tags='a_tag', console_output=True, command_prefix=command_prefix, extra_options=server_options)
    proc2 = driver.Process(cluster=cluster, name='b', server_tags='b_tag', console_output=True, command_prefix=command_prefix, extra_options=server_options)
    
    # This is necessary because a few log messages may be printed even after `wait_until_ready()` returns.
    time.sleep(5.0)

    conn = r.connect(proc1.host, proc1.driver_port)
    tables = ["cluster_config", "db_config", "current_issues", "logs", "server_config", "server_status", "table_config", "table_status"]
    feeds = { }
    for name in tables:
        feeds[name] = AsyncChangefeed(proc1, r.db('rethinkdb').table(name))

    def check(expected, timer):
        time.sleep(timer)
        for name, feed in feeds.iteritems():
            feed.check()
            if name in expected:
                assert len(feed.changes) > 0, "Expected changes on %s, found none." % name
                feed.changes = []
            else:
                assert len(feed.changes) == 0, "Expected no changes on %s, found %s." % (name, feed.changes)
    check([], 5.0)

    utils.print_with_time("Changing auth key...")
    res = r.db("rethinkdb").table("cluster_config").get("auth").update({"auth_key": "foo"}).run(conn)
    assert res["replaced"] == 1 and res["errors"] == 0, res
    res = r.db("rethinkdb").table("cluster_config").get("auth").update({"auth_key": None}).run(conn)
    check(["cluster_config"], 1.0)

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
    feeds["test_config"] = AsyncChangefeed(proc1, r.table('test').config())
    feeds["test_status"] = AsyncChangefeed(proc1,  r.table('test').status())
    feeds["test2_config"] = AsyncChangefeed(proc1, r.table('test2').config())
    feeds["test2_status"] = AsyncChangefeed(proc1, r.table('test2').status())
    
    utils.print_with_time("Adding replicas...")
    res = r.table("test").config().update({"shards": [{"primary_replica": "a", "replicas": ["a", "b"]}]}).run(conn)
    assert res["errors"] == 0, res
    r.table("test").wait().run(conn)
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
