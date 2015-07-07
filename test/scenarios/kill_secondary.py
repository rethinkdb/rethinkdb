#!/usr/bin/env python
# Copyright 2010-2015 RethinkDB, all rights reserved.

import os, sys, time
from pprint import pformat

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, rdb_unittest, scenario_common, utils, vcoptparse, workload_runner
from utils import print_with_time

numNodes = 3

op = vcoptparse.OptParser()
workload_runner.prepare_option_parser_for_split_or_continuous_workload(op)
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)
_, command_prefix, server_options = scenario_common.parse_mode_flags(opts)

print_with_time("Starting cluster of %d servers" % numNodes)
class KillSecondary(rdb_unittest.RdbTestCase):
    
    shards = 1
    replicas = numNodes
    
    server_command_prefix = command_prefix
    server_extra_options = server_options
    
    def test_kill_secondary(self):
        
        primary = self.getPrimaryForShard(0)
        secondary = self.getReplicaForShard(0)
        
        conn = self.r.connect(host=primary.host, port=primary.driver_port)
        
        issues = list(self.r.db('rethinkdb').table('current_issues').run(conn))
        self.assertEqual(issues, [], 'The issues list was not empty: %r' % issues)

        workload_ports = workload_runner.RDBPorts(host=primary.host, http_port=primary.http_port, rdb_port=primary.driver_port, db_name=self.dbName, table_name=self.tableName)
        with workload_runner.SplitOrContinuousWorkload(opts, workload_ports) as workload:
            
            print_with_time("Starting workload")
            workload.run_before()
            
            self.cluster.check()
            issues = list(self.r.db('rethinkdb').table('current_issues').run(conn))
            self.assertEqual(issues, [], 'The issues list was not empty: %r' % issues)
        
            print_with_time("Killing the secondary")
            secondary.kill()
            
            print_with_time("Checking that the table_availability issue shows up")
            deadline = time.time() + 5
            last_error = None
            while time.time() < deadline:
                try:
                    issues = list(self.r.db('rethinkdb').table('current_issues').filter({'type':'table_availability', 'info':{'db':self.dbName, 'table':self.tableName}}).run(conn))
                    self.assertEqual(len(issues), 1, 'The server did not record the single issue for the killed secondary server:\n%s' % pformat(issues))
                    
                    issue = issues[0]
                    self.assertEqual(issue['critical'], False)
                    self.assertEqual(issue['info']['status']['ready_for_reads'], True)
                    self.assertEqual(issue['info']['status']['ready_for_writes'], True)
                    
                    break
                except Exception as e:
                    last_error = e
                    time.sleep(.2)
            else:
                raise last_error
            
            print_with_time("Running after workload")
            workload.run_after()
        print_with_time("Done")

# ==== main

if __name__ == '__main__':
    import unittest
    unittest.main(argv=[sys.argv[0]])
