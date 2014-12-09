#!/usr/bin/env python
# Copyright 2014 RethinkDB, all rights reserved.

from __future__ import print_function

import os, pprint, sys, time, traceback

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

r = utils.import_python_driver()

num_servers = 5

print("Starting cluster of %d servers (%.2fs)" % (num_servers, time.time() - startTime))
with driver.Cluster(output_folder='.') as cluster:
    
    for i in xrange(1, num_servers+1):
        driver.Process(cluster=cluster, files="s%d" % i, server_tags=["tag_%d" % i], command_prefix=command_prefix, extra_options=serve_options)
    cluster.wait_until_ready()
    cluster.check()

    print("Establishing ReQl connections (%.2fs)" % (time.time() - startTime))

    conn = r.connect(host=cluster[0].host, port=cluster[0].driver_port)
    
    print("Renaming a single server (%.2fs)" % (time.time() - startTime))
    
    res = r.db("rethinkdb").table("server_config").filter({"name": "s1"}).update({"name": "foo"}).run(conn)
    assert res["replaced"] == 1 and res["errors"] == 0
    pprint.pprint(list(r.db("rethinkdb").table("server_config").run(conn)))
    assert r.db("rethinkdb").table("server_config").filter({"name": "foo"}).count().run(conn) == 1
    
    print("Renaming all servers (%.2fs)" % (time.time() - startTime))
    res = r.db("rethinkdb").table("server_config").update({"name": "bar"}).run(conn)
    assert res["replaced"] == 1 and res["errors"] == num_servers - 1, repr(res)

    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))
