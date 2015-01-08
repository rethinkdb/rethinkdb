#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

from __future__ import print_function

import sys, os, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse, workload_runner

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
op["use-proxy"] = vcoptparse.BoolFlag("--use-proxy")
op["num-nodes"] = vcoptparse.IntFlag("--num-nodes", 3)
op["num-shards"] = vcoptparse.IntFlag("--num-shards", 2)
op["workload"] = vcoptparse.PositionalArg()
op["timeout"] = vcoptparse.IntFlag("--timeout", 1200)
opts = op.parse(sys.argv)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()

print("Starting cluster of %d servers (%.2fs)" % (opts["num-nodes"], time.time() - startTime))
with driver.Cluster(initial_servers=opts["num-nodes"], output_folder='.', wait_until_ready=False, command_prefix=command_prefix, extra_options=serve_options) as cluster:

    if opts["use-proxy"]:
        driver.ProxyProcess(cluster, 'proxy-logfile', console_output='proxy-output', command_prefix=command_prefix, extra_options=serve_options)
    
    cluster.wait_until_ready()
    
    print("Establishing ReQL connection (%.2fs)" % (time.time() - startTime))
    
    server = cluster[0]
    conn = r.connect(host=server.host, port=server.driver_port)
    
    print("Creating db/table %s/%s (%.2fs)" % (dbName, tableName, time.time() - startTime))
    
    if dbName not in r.db_list().run(conn):
        r.db_create(dbName).run(conn)
    
    if tableName in r.db(dbName).table_list().run(conn):
        r.db(dbName).table_drop(tableName).run(conn)
    r.db(dbName).table_create(tableName).run(conn)
    
    print("Splitting into %d shards (%.2fs)" % (opts["num-shards"], time.time() - startTime))
    
    r.db(dbName).table(tableName).reconfigure(shards=opts["num-shards"], replicas=1).run(conn)
    r.db(dbName).wait().run(conn)
    
    print("Starting workload: %s (%.2fs)" % (opts["workload"], time.time() - startTime))
    
    workloadServer = server if not opts["use-proxy"] else cluster[-1]
    workload_ports = workload_runner.RDBPorts(host=workloadServer.host, http_port=workloadServer.http_port, rdb_port=workloadServer.driver_port, db_name=dbName, table_name=tableName)
    workload_runner.run(opts["workload"], workload_ports, opts["timeout"])

    print("Ended workload: %s (%.2fs)" % (opts["workload"], time.time() - startTime))
print("Done (%.2fs)" % (time.time() - startTime))
