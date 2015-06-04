#!/usr/bin/env python
# Copyright 2010-2015 RethinkDB, all rights reserved.

from __future__ import print_function

import os, sys, time
from pprint import pformat

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, workload_runner, rdb_unittest, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
workload_runner.prepare_option_parser_for_split_or_continuous_workload(op)
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)
_, command_prefix, server_options = scenario_common.parse_mode_flags(opts)

dbName, tableName = utils.get_test_db_table()

numNodes = 3

startTime = time.time()
def print(*args, **kwargs): # add timing information to all print statements
    args += ('(T+ %.2fs)' % (time.time() - startTime), )
    __builtins__.print(*args, **kwargs)

print("Starting cluster of %d servers" % numNodes)
class FailoverToSecondary(rdb_unittest.RdbTestCase):
    
    shards = 1
    replicas = numNodes
    
    server_command_prefix = command_prefix
    server_extra_options = server_options
    
    def test_failover(self):
        '''Run a workload while killing a server to cause a failover to a secondary'''
        
        # - setup
        
        primary = self.getPrimaryForShard(0)
        stable = self.getReplicaForShard(0)
        
        stableConn = self.r.connect(host=stable.host, port=stable.driver_port)
        
        workload_ports = workload_runner.RDBPorts(host=stable.host, http_port=stable.http_port, rdb_port=stable.driver_port, db_name=dbName, table_name=tableName)
        
        # - run test
        
        with workload_runner.SplitOrContinuousWorkload(opts, workload_ports) as workload:
            
            print("Starting workload before")
            workload.run_before()
            self.cluster.check()
            issues = list(self.r.db('rethinkdb').table('current_issues').run(stableConn))
            self.assertEqual(issues, [], 'The server recorded the following issues after the run_before:\n%s' % pformat(issues))
            
            print("Killing the primary")
            primary.kill()
            
            print("Checking that the table_availability issue shows up")
            deadline = time.time() + 5
            last_error = None
            while time.time() < deadline:
                try:
                    issues = list(self.r.db('rethinkdb').table('current_issues').filter({'type':'table_availability', 'info':{'db':dbName, 'table':tableName}}).run(stableConn))
                    self.assertEqual(len(issues), 1, 'The server did not record the single issue for the killed server:\n%s' % pformat(issues))
                    break
                except Exception as e:
                    last_error = e
                    time.sleep(.2)
            else:
                raise last_error
            
            print("Watch for the table to become available again")
            deadline = time.time() + 30
            last_error = None
            while time.time() < deadline:
                try:
                    status = self.table.status().run(stableConn)
                    self.assertEqual(status['status']['ready_for_reads'], True, 'The table never became ready_for_reads:\n%s' % pformat(status))
                    self.assertEqual(status['status']['ready_for_writes'], True, 'The table never became ready_for_writes:\n%s' % pformat(status))
                    break
                except Exception as e:
                    last_error = e
                    time.sleep(.2)
            else:
                raise last_error
        
            print("Running workload after")
            workload.run_after()
            
        print("Cleaning up")

# ==== main

if __name__ == '__main__':
    import unittest
    unittest.main(argv=[sys.argv[0]])
