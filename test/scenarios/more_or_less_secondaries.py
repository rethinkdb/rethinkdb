#!/usr/bin/env python
# Copyright 2010-2016 RethinkDB, all rights reserved.

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
_, command_prefix, server_options = scenario_common.parse_mode_flags(opts)

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()

numReplicas = opts["sequence"].peak()

with driver.Cluster(output_folder='.', initial_servers=numReplicas + 1, console_output=True, command_prefix=command_prefix, extra_options=server_options) as cluster:
    primary = cluster[0]
    replicaPool = cluster[1:]
    
    utils.print_with_time('Establishing ReQL connection')
    
    conn = r.connect(host=primary.host, port=primary.driver_port)
    
    utils.print_with_time('Creating db/table %s/%s' % (dbName, tableName))
    
    if dbName not in r.db_list().run(conn):
        r.db_create(dbName).run(conn)
    if tableName in r.db(dbName).table_list().run(conn):
        r.db(dbName).table_drop(tableName).run(conn)
    r.db(dbName).table_create(tableName).run(conn)
    
    utils.print_with_time('Setting inital table replication settings')
    res = r.db(dbName).table(tableName).config()\
        .update({'shards':[{'primary_replica':primary.name, 'replicas':[primary.name, replicaPool[0].name]}
        ]}).run(conn)
    assert res['errors'] == 0, res
    
    r.db(dbName).wait(wait_for="all_replicas_ready").run(conn)
    cluster.check()
    issues = list(r.db('rethinkdb').table('current_issues').filter(r.row["type"] != "memory_error").run(conn))
    assert len(issues) == 0, 'There were issues on the server: %s' % str(issues)
    
    utils.print_with_time('Starting workload')
    
    workload_ports = workload_runner.RDBPorts(host=primary.host, http_port=primary.http_port, rdb_port=primary.driver_port, db_name=dbName, table_name=tableName)
    with workload_runner.SplitOrContinuousWorkload(opts, workload_ports) as workload:
        
        workload.run_before()
        
        cluster.check()
        res = list(r.db('rethinkdb').table('current_issues').filter(r.row["type"] != "memory_error").run(conn))
        assert res == [], 'There were unexpected issues: \n%s' % utils.RePrint.pformat(res)
        workload.check()
        
        current = opts["sequence"].initial
        for i, s in enumerate(opts["sequence"].steps):
            if i != 0:
                workload.run_between()
            utils.print_with_time("Changing the number of secondaries from %d to %d" % (current, current + s))
            current += s
            
            assert r.db(dbName).table(tableName).config() \
                .update({'shards':[
                    {'primary_replica':primary.name,
                     'replicas':[primary.name] + [x.name for x in replicaPool[:current]]}
                ]}).run(conn)['errors'] == 0
            r.db(dbName).wait(wait_for="all_replicas_ready").run(conn) # ToDo: add timeout when avalible
            
            cluster.check()
            res = list(r.db('rethinkdb').table('current_issues').filter(r.row["type"] != "memory_error").run(conn))
            assert res == [], 'There were unexpected issues after step %d: \n%s' % (i, utils.RePrint.pformat(res))
        workload.run_after()
    
    res = list(r.db('rethinkdb').table('current_issues').filter(r.row["type"] != "memory_error").run(conn))
    assert res == [], 'There were unexpected issues after all steps: \n%s' % utils.RePrint.pformat(res)
    
    utils.print_with_time('Cleaning up')
utils.print_with_time('Done.')
