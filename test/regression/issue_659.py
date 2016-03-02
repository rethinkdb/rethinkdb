#!/usr/bin/env python
# Copyright 2012-2016 RethinkDB, all rights reserved.

'''Lowering the number of replicas results in strange behavior'''

from __future__ import print_function

import os, sys, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import rdb_unittest, utils

class ReplicaChangeRegrssion(rdb_unittest.RdbTestCase):
    shards  = 1
    servers = 2
    
    recordsToGenerate = 20000
    
    def test_replicaChanges(self):
        
        print() # solve a formatting issue with unittest reporting
        tableUUID = self.table.info()['id'].run(self.conn)
        
        utils.print_with_time("Increasing replication factor")
        self.table.reconfigure(shards=1, replicas=2).run(self.conn)
        self.table.wait(wait_for='all_replicas_ready').run(self.conn)
        self.checkCluster()
        
        utils.print_with_time("Checking that both servers have a data file")
        deadline = time.time() + 5
        lastError = None
        while time.time() < deadline:
            for server in self.cluster:
                dataPath = os.path.join(server.data_path, tableUUID)
                if not os.path.exists(dataPath):
                    lastError = 'Server %s does not have a data file at %s' % (server.name, dataPath)
                    break
            else:
                break
        else:
            raise Exception(lastError or 'Timed out in a weird way')
        
        master = self.getPrimaryForShard(0)
        slave = self.getReplicaForShard(0)
        
        utils.print_with_time("Decreasing replication factor")
        self.table.config().update({'shards':[{'primary_replica':master.name, 'replicas':[master.name]}]}).run(self.conn)
        self.table.wait(wait_for='all_replicas_ready').run(self.conn)
        self.checkCluster()
        
        utils.print_with_time("Waiting for file deletion on the slave")
        deadline = time.time() + 5
        slaveDataPath = os.path.join(slave.data_path, tableUUID)
        while time.time() < deadline:
            if not os.path.exists(slaveDataPath):
                break
        else:
            raise Exception('File deletion had not happend after 5 seconds, file still exists at: %s' % slaveDataPath)
        
        utils.print_with_time("Increasing replication factor again")
        self.table.reconfigure(shards=1, replicas=2).run(self.conn, noreply=True)
        
        utils.print_with_time("Confirming that the progress meter indicates a backfill happening")
        deadline = time.time() + 5
        last_error = None
        while time.time() < deadline:
            try:
                assert r.db("rethinkdb") \
                    .table("jobs") \
                    .filter({"type": "backfill", "info": {"table": self.tableName}}) \
                    .count() \
                    .run(self.conn) == 1, "No backfill job found in `rethinkdb.jobs`."
                break
            except Exception, e:
                last_error = e
            time.sleep(0.02)
        else:
            pass #raise last_error
        
        utils.print_with_time("Killing the cluster")
        # The large backfill might take time, and for this test we don't care about it succeeding
        for server in self.cluster[:]:
            server.kill()

if __name__ == '__main__':
    rdb_unittest.main()
