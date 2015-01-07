#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

from __future__ import print_function

import os, sys, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, workload_runner, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
workload_runner.prepare_option_parser_for_split_or_continuous_workload(op)
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()

numNodes = 2

print("Starting cluster of %d servers (%.2fs)" % (numNodes, time.time() - startTime))
with driver.Cluster(initial_servers=numNodes, output_folder='.', wait_until_ready=True, command_prefix=command_prefix, extra_options=serve_options) as cluster:
    
    print('Establishing ReQL connection (%.2fs)' % (time.time() - startTime))
    
    primary = cluster[0]
    secondary = cluster[1]
    conn1 = r.connect(host=primary.host, port=primary.driver_port)
    conn2 = r.connect(host=secondary.host, port=secondary.driver_port)
    
    workload_ports = workload_runner.RDBPorts(host=secondary.host, http_port=secondary.http_port, rdb_port=secondary.driver_port, db_name=dbName, table_name=tableName)
    
    print("Creating db/table %s/%s (%.2fs)" % (dbName, tableName, time.time() - startTime))
    
    if dbName not in r.db_list().run(conn1):
        r.db_create(dbName).run(conn1)
    
    if tableName in r.db(dbName).table_list().run(conn1):
        r.db(dbName).table_drop(tableName).run(conn1)
    r.db(dbName).table_create(tableName).run(conn1)
    
    print("Pinning table to first server (%.2fs)" % (time.time() - startTime))
    
    assert r.db(dbName).table(tableName).config() \
        .update({'shards':[
            {'primary_replica':primary.name, 'replicas':[primary.name, secondary.name]}
        ]}).run(conn1)['errors'] == 0
    r.db(dbName).wait().run(conn1)
    
    print("Starting workload before (%.2fs)" % (time.time() - startTime))
    
    with workload_runner.SplitOrContinuousWorkload(opts, workload_ports) as workload:
        workload.run_before()
        cluster.check()
        issues = list(r.db('rethinkdb').table('issues').run(conn1))
        assert len(issues) == 0, 'The server recorded the following issues: %s' % str(issues)
        
        print("Killing the primary (%.2fs)" % (time.time() - startTime))
        primary.close()
        
        print("Moving the shard to the secondary (%.2fs)" % (time.time() - startTime))
        
        assert r.db(dbName).table(tableName).config() \
            .update({'shards':[
                {'primary_replica':secondary.name, 'replicas':[secondary.name]}
            ]}).run(conn2)['errors'] == 0
        r.db(dbName).wait().run(conn2)
        cluster.check()
        issues = list(r.db('rethinkdb').table('issues').run(conn2))
        assert len(issues) > 0, 'The server did not record the issue for the killed server'
        assert len(issues) == 1, 'The server recorded more issues than the single one expected: %s' % str(issues)
        
        print("Running workload after (%.2fs)" % (time.time() - startTime))
        workload.run_after()
        
        print("Declaring the primary dead (%.2fs)" % (time.time() - startTime))
        
        assert r.db('rethinkdb').table('server_config').get(primary.uuid).delete().run(conn2)['errors'] == 0
        time.sleep(.1)
        issues = list(r.db('rethinkdb').table('issues').run(conn2))
        assert [] == issues, 'The issues list was not empty: %s' % repr(issues)

    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))
