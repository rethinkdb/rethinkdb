#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

from __future__ import print_function

import sys, os, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, utils

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'scenarios')))
import rdb_workload_common

numNodes = 2

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()

print("Starting cluster of %d servers (%.2fs)" % (numNodes, time.time() - startTime))
with driver.Cluster(initial_servers=numNodes, output_folder='.', wait_until_ready=True) as cluster:
    
    server = cluster[0]
    conn = r.connect(host=server.host, port=server.driver_port)
    
    print("Creating db/table %s/%s (%.2fs)" % (dbName, tableName, time.time() - startTime))
    
    if dbName not in r.db_list().run(conn):
        r.db_create(dbName).run(conn)
    
    if tableName in r.db(dbName).table_list().run(conn):
        r.db(dbName).table_drop(tableName).run(conn)
    r.db(dbName).table_create(tableName).run(conn)
    
    print("Inserting some data (%.2fs)" % (time.time() - startTime))
    rdb_workload_common.insert_many(host=server.host, port=server.driver_port, database=dbName, table=tableName, count=10000)
    cluster.check()
    
    print("Splitting into two shards (%.2fs)" % (time.time() - startTime))
    r.db(dbName).table(tableName).reconfigure(shards=2, replicas=2).run(conn)
    r.db(dbName).table_wait().run(conn)
    cluster.check()

    print("Merging shards together again (%.2fs)" % (time.time() - startTime))
    r.db(dbName).table(tableName).reconfigure(shards=1, replicas=1).run(conn)
    r.db(dbName).table_wait().run(conn)
    cluster.check()
    
    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))
