#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

from __future__ import print_function

import os, sys, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, rdb_workload_common, scenario_common, utils, vcoptparse, workload_runner

def sequence_from_string(string):
    returnValue = tuple()
    for step in string.split(","):
        try:
            returnValue += (int(step),)
        except:
            workingValue = returnValue[-1]
            for char in step:
                if char == '-':
                    workingValue -= 1
                elif char == '+':
                    workingValue += 1
                else:
                    raise ValueError('Got a bad step value: %s' % repr(char))
                assert char in ('-', '+')
            returnValue += (workingValue,)
    return returnValue

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
workload_runner.prepare_option_parser_for_split_or_continuous_workload(op, allow_between = True)
op["num-nodes"] = vcoptparse.IntFlag("--num-nodes", 3)
op["sequence"] = vcoptparse.ValueFlag("--sequence", converter=sequence_from_string, default=(2, 3))
opts = op.parse(sys.argv)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)

alphanum = "0123456789abcdefghijklmnopqrstuvwxyz"
candidate_shard_boundaries = set(alphanum).union([x + "9" for x in alphanum]).union([x + "i" for x in alphanum]).union([x + "r" for x in alphanum])

numNodes = opts["num-nodes"]

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()

print("Starting cluster of %d servers (%.2fs)" % (numNodes, time.time() - startTime))
with driver.Cluster(initial_servers=numNodes, output_folder='.', wait_until_ready=True, command_prefix=command_prefix, extra_options=serve_options) as cluster:
    
    print("Establishing ReQL connection (%.2fs)" % (time.time() - startTime))
    
    server = cluster[0]
    conn = r.connect(host=server.host, port=server.driver_port)
    
    print("Creating db/table %s/%s (%.2fs)" % (dbName, tableName, time.time() - startTime))
    
    if dbName not in r.db_list().run(conn):
        r.db_create(dbName).run(conn)
    
    if tableName in r.db(dbName).table_list().run(conn):
        r.db(dbName).table_drop(tableName).run(conn)
    r.db(dbName).table_create(tableName).run(conn)
    
    print("Sharding table (%.2fs)" % (time.time() - startTime))
    
    r.db(dbName).reconfigure(shards=numNodes, replicas=numNodes).run(conn)
    r.db(dbName).wait().run(conn)
    
    print("Inserting data (%.2fs)" % (time.time() - startTime))
    
    rdb_workload_common.insert_many(host=server.host, port=server.driver_port, database=dbName, table=tableName, count=10000, conn=conn)
    
    print("Starting workload (%.2fs)" % (time.time() - startTime))
    
    workload_ports = workload_runner.RDBPorts(host=server.host, http_port=server.http_port, rdb_port=server.driver_port, db_name=dbName, table_name=tableName)
    with workload_runner.SplitOrContinuousWorkload(opts, workload_ports) as workload:
        
        print("Running workload before (%.2fs)" % (time.time() - startTime))
        workload.run_before()
        cluster.check()
        
        currentShards = numNodes
        for currentShards in opts["sequence"]:
            print("Sharding table to %d shards (%.2fs)" % (currentShards, time.time() - startTime))
            
            r.db(dbName).reconfigure(shards=currentShards, replicas=numNodes).run(conn)
            r.db(dbName).wait().run(conn)
            cluster.check()
            assert [] == list(r.db('rethinkdb').table('issues').run(conn))
        
        print("Running workload after (%.2fs)" % (time.time() - startTime))
        workload.run_after()
    
    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))
