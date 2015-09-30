#!/usr/bin/env python
# Copyright 2010-2015 RethinkDB, all rights reserved.

import os, sys, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, rdb_workload_common, scenario_common, utils, vcoptparse, workload_runner

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
op["workload1"] = vcoptparse.StringFlag("--workload-before", None)
op["workload2"] = vcoptparse.StringFlag("--workload-after", None)
op["timeout"] = vcoptparse.IntFlag("--timeout", 600)
opts = op.parse(sys.argv)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()

utils.print_with_time("Starting cluster with one server")
with driver.Cluster(initial_servers=['first'], output_folder='.', command_prefix=command_prefix, extra_options=serve_options, wait_until_ready=True) as cluster:
    
    server1 = cluster[0]
    workload_ports1 = workload_runner.RDBPorts(host=server1.host, http_port=server1.http_port, rdb_port=server1.driver_port, db_name=dbName, table_name=tableName)
    
    utils.print_with_time("Establishing ReQL connection")
    
    conn1 = r.connect(server1.host, server1.driver_port)
    
    utils.print_with_time("Creating db/table %s/%s" % (dbName, tableName))
    
    if dbName not in r.db_list().run(conn1):
        r.db_create(dbName).run(conn1)
    
    if tableName in r.db(dbName).table_list().run(conn1):
        r.db(dbName).table_drop(tableName).run(conn1)
    r.db(dbName).table_create(tableName).run(conn1)

    utils.print_with_time("Starting first workload")
    
    workload_runner.run(opts["workload1"], workload_ports1, opts["timeout"])
    
    utils.print_with_time("Bringing up new server")
    
    server2 = driver.Process(cluster=cluster, name='second', command_prefix=command_prefix, extra_options=serve_options, wait_until_ready=True)
    
    issues = list(r.db('rethinkdb').table('current_issues').run(conn1))
    assert [] == issues, 'The issues list was not empty: %s' % repr(issues)
    
    utils.print_with_time("Explicitly adding server to the table")
    assert r.db(dbName).table(tableName).config() \
        .update({'shards':[
            {'primary_replica':server2.name, 'replicas':[server2.name, server1.name]}
        ]})['errors'].run(conn1) == 0
    
    utils.print_with_time("Waiting for backfill")
    
    r.db(dbName).wait().run(conn1)
    
    utils.print_with_time("Removing the first server from the table")
    assert r.db(dbName).table(tableName).config() \
        .update({'shards':[
            {'primary_replica':server2.name, 'replicas':[server2.name]}
        ]})['errors'].run(conn1) == 0
    r.db(dbName).table(tableName).wait().run(conn1)
    
    utils.print_with_time("Shutting down first server")
    
    server1.check_and_stop()
    time.sleep(.1)
    
    utils.print_with_time("Starting second workload")

    workload_ports2 = workload_runner.RDBPorts(host=server2.host, http_port=server2.http_port, rdb_port=server2.driver_port, db_name=dbName, table_name=tableName)
    workload_runner.run(opts["workload2"], workload_ports2, opts["timeout"])

    utils.print_with_time("Cleaning up")
utils.print_with_time("Done.")
