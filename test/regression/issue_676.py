#!/usr/bin/env python
# Copyright 2012-2016 RethinkDB, all rights reserved.

'''Merging shards causes a server crash'''

from __future__ import print_function

import sys, os

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, utils

numNodes = 2

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()

utils.print_with_time("Starting cluster of %d servers" % numNodes)
with driver.Cluster(initial_servers=numNodes, output_folder='.', wait_until_ready=True) as cluster:
    
    server = cluster[0]
    conn = r.connect(host=server.host, port=server.driver_port)
    
    utils.print_with_time("Creating db/table %s/%s" % (dbName, tableName))
    
    if dbName not in r.db_list().run(conn):
        r.db_create(dbName).run(conn)
    
    if tableName in r.db(dbName).table_list().run(conn):
        r.db(dbName).table_drop(tableName).run(conn)
    r.db(dbName).table_create(tableName).run(conn)
    tbl = r.db(dbName).table(tableName)
    
    utils.print_with_time("Inserting some data")
    utils.populateTable(conn, tbl, records=10000)
    cluster.check()
    
    utils.print_with_time("Splitting into two shards")
    tbl.reconfigure(shards=2, replicas=2).run(conn)
    r.db(dbName).wait().run(conn)
    cluster.check()

    utils.print_with_time("Merging shards together again")
    tbl.reconfigure(shards=1, replicas=1).run(conn)
    r.db(dbName).wait().run(conn)
    cluster.check()
    
    utils.print_with_time("Cleaning up")
utils.print_with_time("Done.")
