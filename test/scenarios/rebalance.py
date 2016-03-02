#!/usr/bin/env python
# Copyright 2010-2015 RethinkDB, all rights reserved.

import os, sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import rdb_unittest, scenario_common, utils, vcoptparse, workload_runner

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

class Rebalance(rdb_unittest.RdbTestCase):
    '''Change the number of shards on a table.'''
    # keep the same number of replicas as nodes so we don't have to backfill constantly
    
    replicas = opts["num-nodes"]
    shards = 1
    
    server_command_prefix = command_prefix
    server_extra_options = serve_options
    
    def test_workload(self):
        
        connServer = self.cluster[0]
        
        utils.print_with_time("Inserting data")
        utils.populateTable(self.conn, self.table, records=10000, fieldName='val')
        
        utils.print_with_time("Starting workload")
        with workload_runner.SplitOrContinuousWorkload(opts, connServer, db_name=self.dbName, table_name=self.tableName) as workload:
            
            utils.print_with_time("Running workload before")
            workload.run_before()
            self.checkCluster()
            
            for currentShards in opts["sequence"]:
                
                utils.print_with_time("Sharding table to %d shards" % currentShards)
                self.table.reconfigure(shards=currentShards, replicas=opts["num-nodes"]).run(self.conn)
                self.table.wait(wait_for='all_replicas_ready').run(self.conn)
                self.checkCluster()
            
            utils.print_with_time("Running workload after")
            workload.run_after()
            self.checkCluster()
        
        utils.print_with_time("Workload complete")

# ===== main

if __name__ == '__main__':
    rdb_unittest.main()
