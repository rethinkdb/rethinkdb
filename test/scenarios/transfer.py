#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

from __future__ import print_function

import os, sys, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, rdb_workload_common, scenario_common, utils, vcoptparse, workload_runner

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
with driver.Cluster(initial_servers=['first'], output_folder='.', command_prefix=command_prefix, extra_options=serve_options, wait_until_ready=True) as cluster:
    
    server1 = cluster[0]
    workload_ports1 = workload_runner.RDBPorts(host=server1.host, http_port=server1.http_port, rdb_port=server1.driver_port, db_name=dbName, table_name=tableName)
    
    print("Establishing ReQL connection (%.2fs)" % (time.time() - startTime))
    
    conn1 = r.connect(server1.host, server1.driver_port)
    
    print("Creating db/table %s/%s (%.2fs)" % (dbName, tableName, time.time() - startTime))
    
    if dbName not in r.db_list().run(conn1):
        r.db_create(dbName).run(conn1)
    
    if tableName in r.db(dbName).table_list().run(conn1):
        r.db(dbName).table_drop(tableName).run(conn1)
    r.db(dbName).table_create(tableName).run(conn1)

    print("Starting first workload (%.2fs)" % (time.time() - startTime))
    
    workload_runner.run(opts["workload1"], workload_ports1, opts["timeout"])
    
    print("Bringing up new server (%.2fs)" % (time.time() - startTime))
    
    server2 = driver.Process(cluster=cluster, files='second', command_prefix=command_prefix, extra_options=serve_options, wait_until_ready=True)
    
    issues = list(r.db('rethinkdb').table('issues').run(conn1))
    assert [] == issues, 'The issues list was not empty: %s' % repr(issues)
    
    print("Explicitly adding server to the table (%.2fs)" % (time.time() - startTime))
    assert r.db(dbName).table(tableName).config() \
        .update({'shards':[
            {'primary_replica':server2.name, 'replicas':[server2.name, server1.name]}
        ]})['errors'].run(conn1) == 0
    
    print("Waiting for backfill (%.2fs)" % (time.time() - startTime))
    
    r.db(dbName).wait().run(conn1)
    
    print("Removing the first server from the table (%.2fs)" % (time.time() - startTime))
    assert r.db(dbName).table(tableName).config() \
        .update({'shards':[
            {'primary_replica':server2.name, 'replicas':[server2.name]}
        ]})['errors'].run(conn1) == 0
    r.db(dbName).wait().run(conn1)
    
    print("Shutting down first server (%.2fs)" % (time.time() - startTime))
    
    server1.check_and_stop()
    time.sleep(.1)
    
    conn2 = r.connect(server2.host, server2.driver_port)
    
    issues = list(r.db('rethinkdb').table('issues').run(conn2))
    assert len(issues) == 1, 'The issues was not the single item expected: %s' % repr(issues)
    
    print("Declaring first server dead (%.2fs)" % (time.time() - startTime))
    
    assert r.db('rethinkdb').table('server_config').get(server1.uuid).delete().run(conn2)['errors'] == 0
    time.sleep(.1)
    r.db(dbName).wait().run(conn2)
    
    issues = list(r.db('rethinkdb').table('issues').run(conn2))
    assert [] == issues, 'The issues list was not empty: %s' % repr(issues)
    
    print("Starting second workload (%.2fs)" % (time.time() - startTime))

    workload_ports2 = workload_runner.RDBPorts(host=server2.host, http_port=server2.http_port, rdb_port=server2.driver_port, db_name=dbName, table_name=tableName)
    workload_runner.run(opts["workload2"], workload_ports2, opts["timeout"])

    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))
