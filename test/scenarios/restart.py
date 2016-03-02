#!/usr/bin/env python
# Copyright 2010-2015 RethinkDB, all rights reserved.

import os, sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import rdb_unittest, scenario_common, utils, vcoptparse, workload_runner

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
op["workload1"] = vcoptparse.StringFlag("--workload-before", None)
op["workload2"] = vcoptparse.StringFlag("--workload-after", None)
op["timeout"] = vcoptparse.IntFlag("--timeout", 600)
opts = op.parse(sys.argv)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)

class ChangePrimary(rdb_unittest.RdbTestCase):
    '''Restart a server to make sure it keeps its data on restart'''
    
    servers = 1
    
    server_command_prefix = command_prefix
    server_extra_options = serve_options
    
    def test_workload(self):
        
        server = self.cluster[0]
        
        utils.print_with_time("Running first workload")
        workload_runner.run(opts["workload1"], server, opts["timeout"], db_name=self.dbName, table_name=self.tableName)
        
        utils.print_with_time("Restarting server")
        server.check_and_stop()
        server.start()
        self.cluster.check()
        self.r.db(self.dbName).wait().run(self.conn)
        
        utils.print_with_time("Running second workload")
        workload_runner.run(opts["workload2"], server, opts["timeout"], db_name=self.dbName, table_name=self.tableName)

# ===== main

if __name__ == '__main__':
    rdb_unittest.main()
