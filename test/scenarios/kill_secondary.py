#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

from __future__ import print_function

import os, sys, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse, workload_runner

op = vcoptparse.OptParser()
workload_runner.prepare_option_parser_for_split_or_continuous_workload(op)
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)

numNodes = 2

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()

print("Starting cluster of %d servers (%.2fs)" % (numNodes, time.time() - startTime))
with driver.Cluster(initial_servers=numNodes, output_folder='.', wait_until_ready=True, command_prefix=command_prefix, extra_options=serve_options) as cluster:
    
    print("Establishing ReQL Connection (%.2fs)" % (time.time() - startTime))
    
    server = cluster[0]
    secondary = cluster[1]
    conn = r.connect(host=server.host, port=server.driver_port)
    
    print("Creating db/table %s/%s (%.2fs)" % (dbName, tableName, time.time() - startTime))
    
    if dbName not in r.db_list().run(conn):
        r.db_create(dbName).run(conn)
    
    if tableName in r.db(dbName).table_list().run(conn):
        r.db(dbName).table_drop(tableName).run(conn)
    r.db(dbName).table_create(tableName).run(conn)
    
    print("Reconfiguring table to have 1 shard but %d replicas (%.2fs)" % (numNodes, time.time() - startTime))
    
    r.db(dbName).table(tableName).reconfigure(shards=1, replicas=numNodes).run(conn)
    assert r.db(dbName).table_config(tableName).update({'shards':[{'director':server.name, 'replicas':r.row['shards'][0]['replicas']}]}).run(conn)['errors'] == 0
    r.db(dbName).table_wait().run(conn)
    
    cluster.check()
    issues = list(r.db('rethinkdb').table('issues').run(conn))
    assert [] == issues, 'The issues list was not empty: %s' % repr(issues)
    
    print("Starting workload (%.2fs)" % (time.time() - startTime))

    workload_ports = workload_runner.RDBPorts(host=server.host, http_port=server.http_port, rdb_port=server.driver_port, db_name=dbName, table_name=tableName)
    with workload_runner.SplitOrContinuousWorkload(opts, workload_ports) as workload:
        workload.run_before()
        cluster.check()
        issues = list(r.db('rethinkdb').table('issues').run(conn))
        assert [] == issues, 'The issues list was not empty: %s' % repr(issues)
        
        print("Killing the secondary (%.2fs)" % (time.time() - startTime))
        
        secondary.kill()
        
        time.sleep(.5)
        issues = list(r.db('rethinkdb').table('issues').run(conn))
        assert len(issues) > 0, 'The dead server issue is not listed:'
        assert len(issues) < 2, 'There are too many issues in the list: %s' % repr(issues)
        
        print("Declaring the server dead (%.2fs)" % (time.time() - startTime))
        
        assert r.db('rethinkdb').table('server_config').get(secondary.uuid).delete().run(conn)['errors'] == 0
        time.sleep(.1)
        issues = list(r.db('rethinkdb').table('issues').run(conn))
        assert [] == issues, 'The issues list was not empty: %s' % repr(issues)
        
        print("Running after workload (%.2fs)" % (time.time() - startTime))
        
        workload.run_after()
    
    cluster.check()
    issues = list(r.db('rethinkdb').table('issues').run(conn))
    assert [] == issues, 'The issues list was not empty: %s' % repr(issues)
    
    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done (%.2fs)" % (time.time() - startTime))
