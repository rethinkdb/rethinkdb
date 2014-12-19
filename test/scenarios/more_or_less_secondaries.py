#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

from __future__ import print_function

import os, sys, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse, workload_runner

class ReplicaSequence(object):
    def __init__(self, string):
        assert set(string) < set("0123456789+-")
        string = string.replace("-", "\0-").replace("+", "\0+")
        parts = string.split("\0")
        assert set(parts[0]) < set("0123456789")
        self.initial = int(parts[0])
        self.steps = [int(x) for x in parts[1:]]
        current = self.initial
        for step in self.steps:
            current += step
            assert current >= 0
    def peak(self):
        peak = current = self.initial
        for step in self.steps:
            current += step
            peak = max(current, peak)
        return peak
    def __repr__(self):
        return str(self.initial) + "".join(("+" if s > 0 else "-") + str(abs(x)) for x in self.steps)

op = vcoptparse.OptParser()
workload_runner.prepare_option_parser_for_split_or_continuous_workload(op, allow_between=True)
scenario_common.prepare_option_parser_mode_flags(op)
op["sequence"] = vcoptparse.PositionalArg(converter=ReplicaSequence)
opts = op.parse(sys.argv)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()

numReplicas = opts["sequence"].peak()

print('Starting cluster (%.2fs)' % (time.time() - startTime))
with driver.Cluster(output_folder='.') as cluster:
    
    print('Starting primary server (%.2fs)' % (time.time() - startTime))
    
    primary_files = driver.Files(cluster.metacluster, db_path="db-primary", console_output=True, command_prefix=command_prefix)
    primary_process = driver.Process(cluster, primary_files, console_output=True, command_prefix=command_prefix, extra_options=serve_options)
    
    print('Starting %d replicas (%.2fs)' % (numReplicas, time.time() - startTime))
    
    replica_processes = [driver.Process(cluster=cluster, console_output=True, command_prefix=command_prefix, extra_options=serve_options) for i in xrange(numReplicas)]
    cluster.wait_until_ready()
    primary = cluster[0]
    replicaPool = cluster[1:]
    
    print('Establishing ReQL connection (%.2fs)' % (time.time() - startTime))
    
    conn = r.connect(host=primary.host, port=primary.driver_port)
    
    print('Creating db/table %s/%s (%.2fs)' % (dbName, tableName, time.time() - startTime))
    
    if dbName not in r.db_list().run(conn):
        r.db_create(dbName).run(conn)
    if tableName in r.db(dbName).table_list().run(conn):
        r.db(dbName).table_drop(tableName).run(conn)
    r.db(dbName).table_create(tableName).run(conn)
    
    print('Setting inital table replication settings (%.2fs)' % (time.time() - startTime))
    
    assert r.db(dbName).table_config(tableName).update({'shards':[{'director':primary.name, 'replicas':[primary.name, replicaPool[0].name]}]}).run(conn)['errors'] == 0
    
    r.db(dbName).table_wait().run(conn)
    cluster.check()
    issues = list(r.db('rethinkdb').table('issues').run(conn))
    assert len(issues) == 0, 'There were issues on the server: %s' % str(issues)
    
    print('Starting workload (%.2fs)' % (time.time() - startTime))
    
    workload_ports = workload_runner.RDBPorts(host=primary.host, http_port=primary.http_port, rdb_port=primary.driver_port, db_name=dbName, table_name=tableName)
    with workload_runner.SplitOrContinuousWorkload(opts, workload_ports) as workload:
        
        workload.run_before()
        
        cluster.check()
        assert list(r.db('rethinkdb').table('issues').run(conn)) == []
        workload.check()
        
        current = opts["sequence"].initial
        for i, s in enumerate(opts["sequence"].steps):
            if i != 0:
                workload.run_between()
            print("Changing the number of secondaries from %d to %d (%.2fs)" % (current, current + s, time.time() - startTime))
            current += s
            
            assert r.db(dbName).table_config(tableName).update({'shards':[{'director':primary.name, 'replicas':[primary.name] + [x.name for x in replicaPool[:current]]}]}).run(conn)['errors'] == 0
            r.db(dbName).table_wait().run(conn) # ToDo: add timeout when avalible
            
            cluster.check()
            assert list(r.db('rethinkdb').table('issues').run(conn)) == []
        workload.run_after()

    assert list(r.db('rethinkdb').table('issues').run(conn)) == []
    
    print('Cleaning up (%.2fs)' % (time.time() - startTime))
print('Done. (%.2fs)' % (time.time() - startTime))
