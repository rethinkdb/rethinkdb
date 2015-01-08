#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

from __future__ import print_function

import sys, os, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse, workload_runner

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
op["workload"] = vcoptparse.PositionalArg()
op["timeout"] = vcoptparse.IntFlag("--timeout", 600)
opts = op.parse(sys.argv)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()

print("Starting server (%.2fs)" % (time.time() - startTime))
with driver.Process(output_folder='.', command_prefix=command_prefix, extra_options=serve_options) as server:
    
    print("Establishing ReQL connection (%.2fs)" % (time.time() - startTime))
    
    conn = r.connect(host=server.host, port=server.driver_port)
    
    print("Starting proxy server (%.2fs)" % (time.time() - startTime))
    
    # remove --cache-size
    for option in serve_options[:]:
        if option == '--cache-size':
            position = serve_options.index(option)
            serve_options.pop(position)
            if len(serve_options) > position: # we have at least one more option... the cache size
                serve_options.pop(position)
            break # we can only handle one
        elif option.startswith('--cache-size='):
            serve_options.remove(option)
    
    proxy_process = driver.ProxyProcess(server.cluster, 'proxy-logfile', console_output='proxy-output', command_prefix=command_prefix, extra_options=serve_options)
    server.cluster.wait_until_ready()
    
    print("Creating db/table %s/%s (%.2fs)" % (dbName, tableName, time.time() - startTime))
    
    if dbName not in r.db_list().run(conn):
        r.db_create(dbName).run(conn)
    if tableName in r.db(dbName).table_list().run(conn):
        r.db(dbName).table_drop(tableName).run(conn)
    r.db(dbName).table_create(tableName).run(conn)
    
    print("Starting workload: %s (%.2fs)" % (opts["workload"], time.time() - startTime))

    workload_ports = scenario_common.get_workload_ports([proxy_process], tableName, dbName)
    workload_runner.run(opts["workload"], workload_ports, opts["timeout"])
    
    print("Ended workload: %s (%.2fs)" % (opts["workload"], time.time() - startTime))
print("Done (%.2fs)" % (time.time() - startTime))
