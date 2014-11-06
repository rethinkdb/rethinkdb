#!/usr/bin/env python
# Copyright 2014 RethinkDB, all rights reserved.

import sys, os, time, traceback, pprint
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

r = utils.import_python_driver()

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    _, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)

    num_servers = 5
    print "Spinning up %d processes..." % num_servers
    files = [driver.Files(
            metacluster, console_output="create-output-%d" % (i+1),
            server_name="s%d" % (i+1), server_tags=["tag_%d" % (i+1)],
            command_prefix=command_prefix)
        for i in xrange(num_servers)]
    procs = [driver.Process(
            cluster, files[i], console_output="serve-output-%d" % (i+1),
            command_prefix=command_prefix, extra_options=serve_options)
        for i in xrange(num_servers)]
    for p in procs:
        p.wait_until_started_up()
    cluster.check()

    conn = r.connect(procs[0].host, procs[0].driver_port)

    print "Renaming a single server..."
    res = r.db("rethinkdb").table("server_config") \
           .filter({"name": "s1"}) \
           .update({"name": "foo"}).run(conn)
    assert res["replaced"] == 1 and res["errors"] == 0
    pprint.pprint(list(
        r.db("rethinkdb").table("server_config").run(conn)))
    assert r.db("rethinkdb").table("server_config") \
            .filter({"name": "foo"}).count().run(conn) == 1

    print "Renaming all servers..."
    res = r.db("rethinkdb").table("server_config").update({"name": "bar"}).run(conn)
    assert res["replaced"] == 1 and res["errors"] == num_servers - 1, repr(res)

    cluster.check_and_stop()
print "Done."

