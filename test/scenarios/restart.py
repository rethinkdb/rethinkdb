#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

from __future__ import print_function

import collections, os, sys, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse, workload_runner

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
op["workload1"] = vcoptparse.PositionalArg()
op["workload2"] = vcoptparse.PositionalArg()
op["timeout"] = vcoptparse.IntFlag("--timeout", 600)
opts = op.parse(sys.argv)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()

print("Starting cluster with one server (%.2fs)" % (time.time() - startTime))
with driver.Cluster(initial_servers=1, output_folder='.', command_prefix=command_prefix, extra_options=serve_options) as cluster:
    server = cluster[0]
    files = server.files
    
    print("Establishing ReQL connection (%.2fs)" % (time.time() - startTime))
    
    conn = r.connect(host=server.host, port=server.driver_port)
    
    print("Creating table (%.2fs)" % (time.time() - startTime))
    
    if not dbName in r.db_list().run(conn):
        r.db_create(dbName).run(conn)
    if tableName in r.db(dbName).table_list().run(conn):
        r.db(dbName).table_drop(tableName).run(conn)
    r.db(dbName).table_create(tableName).run(conn)
    
    print("Running first workload (%.2fs)" % (time.time() - startTime))
    
    workload_ports = scenario_common.get_workload_ports([server], tableName, dbName)
    workload_runner.run(opts["workload1"], workload_ports, opts["timeout"])
    
    print("Restarting server (%.2fs)" % (time.time() - startTime))
    server.check_and_stop()
    serverRestarted = driver.Process(cluster, files, command_prefix=command_prefix, extra_options=serve_options, wait_until_ready=True)
    
    conn = r.connect(host=serverRestarted.host, port=serverRestarted.driver_port)
    r.db(dbName).table(tableName).wait().run(conn)
    cluster.check()
    
    print("Running second workload (%.2fs)" % (time.time() - startTime))
    
    workload_ports2 = scenario_common.get_workload_ports([serverRestarted], tableName, dbName)
    workload_runner.run(opts["workload2"], workload_ports2, opts["timeout"])
    
    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))
