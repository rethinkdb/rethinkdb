#!/usr/bin/env python
# Copyright 2010-2016 RethinkDB, all rights reserved.

'''Check that sharding then re-merging keeps all data'''

import os, sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, utils

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()

numNodes = 2
numShards = 2
numReplicas = 2
numRecords = 500

utils.print_with_time("Starting cluster of %d servers" % numNodes)
with driver.Cluster(initial_servers=numNodes, output_folder='.', wait_until_ready=True) as cluster:
    
    utils.print_with_time("Establishing ReQL connection")
    server = cluster[0]
    conn = r.connect(host=server.host, port=server.driver_port)
    
    utils.print_with_time("Creating db/table %s/%s" % (dbName, tableName))
    if dbName not in r.db_list().run(conn):
        r.db_create(dbName).run(conn)
    if tableName in r.db(dbName).table_list().run(conn):
        r.db(dbName).table_drop(tableName).run(conn)
    r.db(dbName).table_create(tableName).run(conn)
    tbl = r.db(dbName).table(tableName)
    
    utils.print_with_time("Adding data to table")
    tbl.insert(r.range(numRecords).map(lambda x: {})).run(conn)

    utils.print_with_time("Splitting into %d shards" % numShards)
    tbl.reconfigure(shards=numShards, replicas=1).run(conn)
    tbl.wait(wait_for='all_replicas_ready').run(conn)
    cluster.check()

    utils.print_with_time("Setting replication factor to %d" % numReplicas)
    tbl.reconfigure(shards=numShards, replicas=numReplicas).run(conn)
    tbl.wait(wait_for='all_replicas_ready').run(conn)
    cluster.check()

    utils.print_with_time("Merging shards together again")
    tbl.reconfigure(shards=1, replicas=numReplicas).run(conn)
    tbl.wait(wait_for='all_replicas_ready').run(conn)
    cluster.check()
    
    utils.print_with_time("Checking that table has the expected number of items")
    assert numRecords == tbl.count().run(conn)
    
    utils.print_with_time("Cleaning up")
utils.print_with_time("Done.")
