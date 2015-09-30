#!/usr/bin/env python
# Copyright 2014-2015 RethinkDB, all rights reserved.

import os, sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

r = utils.import_python_driver()

num_servers = 5

utils.print_with_time("Starting cluster of %d servers" % num_servers)
with driver.Cluster(output_folder='.', initial_servers=["s%d" % i for i in range(1, num_servers+1)]) as cluster:
    
    utils.print_with_time("Establishing ReQl connections")
    conn = r.connect(host=cluster[0].host, port=cluster[0].driver_port)
    
    utils.print_with_time("Renaming a single server")
    res = r.db("rethinkdb").table("server_config").filter({"name": "s1"}).update({"name": "foo"}).run(conn)
    assert res["replaced"] == 1 and res["errors"] == 0
    assert r.db("rethinkdb").table("server_config").filter({"name": "foo"}).count().run(conn) == 1
    
    utils.print_with_time("Renaming all servers")
    res = r.db("rethinkdb").table("server_config").update({"name": "bar"}).run(conn)
    assert res["replaced"] == 1 and res["errors"] == num_servers - 1, repr(res)

    utils.print_with_time("Cleaning up")
utils.print_with_time("Done.")
