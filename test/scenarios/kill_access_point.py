#!/usr/bin/env python
# Copyright 2010-2015 RethinkDB, all rights reserved.

import os, pprint, sys, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import rdb_unittest, scenario_common, utils, vcoptparse, workload_runner

op = vcoptparse.OptParser()
op["workload"] = vcoptparse.PositionalArg()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)

class KillIdleServer(rdb_unittest.RdbTestCase):
    '''Start two servers, with data on only one kill and remove the second and verify that the primary continues'''
    
    servers = 2
    shards = 1
    replicas = 1
    
    server_command_prefix = command_prefix
    server_extra_options = serve_options
    
    def test_workload(self):
        
        primary = self.getPrimaryForShard(0)
        idleServer = None
        for server in self.cluster:
            if server not in self.getReplicasForShard(0) + [primary]:
                idleServer = server
                break
        else:
            raise Exception('Could not find a server that was not serving this table')
        assert primary != idleServer
        
        with workload_runner.ContinuousWorkload(opts["workload"], primary, db_name=self.dbName, table_name=self.tableName) as workload:
            
            utils.print_with_time("Starting workload and running for 10 seconds")
            workload.start()
            time.sleep(10)
            self.checkCluster
            
            utils.print_with_time("Killing the idle server")
            idleServer.kill() # this removes it from the cluster
            self.checkCluster
            
            utils.print_with_time("Running workload for 10 more seconds")
            time.sleep(10)
            self.checkCluster
        
        utils.print_with_time("Done with kill_access_point")

# ==== main

if __name__ == '__main__':
    import unittest
    unittest.main(argv=[sys.argv[0]])
