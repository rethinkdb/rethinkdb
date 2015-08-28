#!/usr/bin/env python
# Copyright 2010-2015 RethinkDB, all rights reserved.

import os, sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, rdb_unittest, scenario_common, utils, vcoptparse, workload_runner

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
op["use-proxy"] = vcoptparse.BoolFlag("--use-proxy")
op["num-shards"] = vcoptparse.IntFlag("--num-shards", 2)
op["workload"] = vcoptparse.PositionalArg()
op["timeout"] = vcoptparse.IntFlag("--timeout", 1200)
opts = op.parse(sys.argv)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)

class StaticCluster(rdb_unittest.RdbTestCase):
    '''Run a workload on an unchanging cluster'''
    
    shards = opts["num-shards"]
    
    server_command_prefix = command_prefix
    server_extra_options = serve_options
    
    def test_workload(self):
        
        workloadServer = self.cluster[0]
        
        # -- add a proxy node if called for
        if opts["use-proxy"]:
            utils.print_with_time('Using proxy')
            workloadServer = driver.ProxyProcess(self.cluster, 'proxy-logfile', console_output='proxy-output', command_prefix=command_prefix, extra_options=serve_options)
            self.cluster.wait_until_ready()
        
        # -- run workload
        workload_runner.run(opts["workload"], workloadServer, opts["timeout"], db_name=self.dbName, table_name=self.tableName)
        utils.print_with_time("Ended workload: %s" % opts["workload"])

# ==== main

if __name__ == '__main__':
    import unittest
    unittest.main(argv=[sys.argv[0]])
