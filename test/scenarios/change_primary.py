#!/usr/bin/env python
# Copyright 2010-2015 RethinkDB, all rights reserved.

import os, pprint, sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import rdb_unittest, scenario_common, utils, vcoptparse, workload_runner

op = vcoptparse.OptParser()
workload_runner.prepare_option_parser_for_split_or_continuous_workload(op)
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)

class ChangePrimary(rdb_unittest.RdbTestCase):
    '''Change the primary from one machine to another while running a workload'''
    
    replicas = 2
    shards = 1
    
    server_command_prefix = command_prefix
    server_extra_options = serve_options
    
    def test_workload(self):
        alpha = self.getPrimaryForShard(0)
        beta = self.getReplicaForShard(0)
        
        workload_ports = workload_runner.RDBPorts(host=alpha.host, http_port=alpha.http_port, rdb_port=alpha.driver_port, db_name=self.dbName, table_name=self.tableName)
        with workload_runner.SplitOrContinuousWorkload(opts, workload_ports) as workload:
            utils.print_with_time('Workloads:\n%s' % pprint.pformat(workload.opts))
            utils.print_with_time("Running before workload")
            workload.run_before()
            utils.print_with_time("Before workload complete")
            self.checkCluster()
            workload.check()
            
            utils.print_with_time("Demoting primary")
            shardConfig = self.table.config()['shards'].run(self.conn)
            shardConfig[0]['primary_replica'] = beta.name
            self.table.config().update({'shards': shardConfig}).run(self.conn)
            self.table.wait(wait_for='all_replicas_ready').run(self.conn)
            self.checkCluster()
            
            utils.print_with_time("Running after workload")
            workload.run_after()
            self.checkCluster()
            utils.print_with_time("After workload complete")
            
# ==== main

if __name__ == '__main__':
    import unittest
    unittest.main(argv=[sys.argv[0]])
