#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

import sys, os, time

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
    
    server1 = cluster[0]
    server2 = cluster[1]
    conn = r.connect(host=server1.host, port=server1.driver_port)
    
    print("Creating db/table %s/%s (%.2fs)" % (dbName, tableName, time.time() - startTime))
    
    if dbName not in r.db_list().run(conn):
        r.db_create(dbName).run(conn)
    
    if tableName in r.db(dbName).table_list().run(conn):
        r.db(dbName).table_drop(tableName).run(conn)
    r.db(dbName).table_create(tableName).run(conn)
    
    print("Setting primary replica to first server (%.2fs)" % (time.time() - startTime))
    
    assert r.db(dbName).table(tableName).config() \
        .update({'shards':[
            {'primary_replica':server1.name, 'replicas':[server1.name, server2.name]}
        ]}).run(conn)['errors'] == 0
    r.db(dbName).wait().run(conn)
    cluster.check()
    assert [] == list(r.db('rethinkdb').table('current_issues').run(conn))
    
    print("Starting workload (%.2fs)" % (time.time() - startTime))
    
    workload_ports = workload_runner.RDBPorts(host=server1.host, http_port=server1.http_port, rdb_port=server1.driver_port, db_name=dbName, table_name=tableName)
    with workload_runner.SplitOrContinuousWorkload(opts, workload_ports) as workload:
        workload.run_before()
        cluster.check()
        assert [] == list(r.db('rethinkdb').table('current_issues').run(conn))
        workload.check()
        
        print("Changing the primary replica to second server (%.2fs)" % (time.time() - startTime))
        
        assert r.db(dbName).table(tableName).config() \
            .update({'shards':[
                {'primary_replica':server2.name, 'replicas':[server1.name, server2.name]}
            ]}).run(conn)['errors'] == 0
        r.db(dbName).wait().run(conn)
        cluster.check()
        assert [] == list(r.db('rethinkdb').table('current_issues').run(conn))
        
        print("Running after workload (%.2fs)" % (time.time() - startTime))
        
        workload.run_after()
    
    cluster.check()
    assert [] == list(r.db('rethinkdb').table('current_issues').run(conn))
    
    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))
