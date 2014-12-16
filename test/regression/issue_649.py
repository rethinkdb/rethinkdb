#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

from __future__ import print_function

import os, sys, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, utils

try:
    xrange
except NameError:
    xrange = range

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()

numNodes = 2
numShards = 2
numReplicas = 2
numRecords = 500

print("Starting cluster of %d servers (%.2fs)" % (numNodes, time.time() - startTime))
with driver.Cluster(initial_servers=numNodes, output_folder='.', wait_until_ready=True) as cluster:
    
    print("Establishing ReQL connection (%.2fs)" % (time.time() - startTime))
    
    server = cluster[0]
    conn = r.connect(host=server.host, port=server.driver_port)
    
    print("Creating db/table %s/%s (%.2fs)" % (dbName, tableName, time.time() - startTime))
    
    if dbName not in r.db_list().run(conn):
        r.db_create(dbName).run(conn)
    
    if tableName in r.db(dbName).table_list().run(conn):
        r.db(dbName).table_drop(tableName).run(conn)
    r.db(dbName).table_create(tableName).run(conn)
    
    print("Adding data to table (%.2fs)" % (time.time() - startTime))
    r.db(dbName).table(tableName).insert(r.range(numRecords).map(lambda x: {})).run(conn)

    print("Splitting into %d shards (%.2fs)" % (numShards, time.time() - startTime))
    r.db(dbName).table(tableName).reconfigure(shards=numShards, replicas=1).run(conn)
    r.db(dbName).wait().run(conn)
    cluster.check()

    print("Setting replication factor to %d (%.2fs)" % (numReplicas, time.time() - startTime))
    r.db(dbName).table(tableName).reconfigure(shards=numShards, replicas=numReplicas).run(conn)
    r.db(dbName).wait().run(conn)
    cluster.check()

    print("Merging shards together again (%.2fs)" % (time.time() - startTime))
    r.db(dbName).table(tableName).reconfigure(shards=1, replicas=numReplicas).run(conn)
    r.db(dbName).wait().run(conn)
    cluster.check()
    
    print("Checking that table has the expected number of items (%.2fs)" % (time.time() - startTime))
    assert numRecords == r.db(dbName).table(tableName).count().run(conn)
    
    print("Cleaning up (%.2fs)" % (time.time() - startTime))

print("Done. (%.2fs)" % (time.time() - startTime))
