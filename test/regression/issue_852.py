#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

from __future__ import print_function

import sys, os, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, rdb_workload_common, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

numNodes = 2

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()

print("Starting cluster of %d servers (%.2fs)" % (numNodes, time.time() - startTime))
with driver.Cluster(initial_servers=numNodes, output_folder='.', wait_until_ready=True, command_prefix=command_prefix, extra_options=serve_options) as cluster:
    
    server1 = cluster[0]
    server2 = cluster[1]
    conn = r.connect(host=server1.host, port=server1.driver_port)
    
    print("Creating db/table %s/%s (%.2fs)" % (dbName, tableName, time.time() - startTime))
    
    if dbName not in r.db_list().run(conn):
        r.db_create(dbName).run(conn)
    
    if tableName in r.db(dbName).table_list().run(conn):
        r.db(dbName).table_drop(tableName).run(conn)
    r.db(dbName).table_create(tableName).run(conn)

    print("Inserting some data (%.2fs)" % (time.time() - startTime))
    rdb_workload_common.insert_many(host=server1.host, port=server1.driver_port, database=dbName, table=tableName, count=10000)
    cluster.check()
    
    print("Splitting into two shards (%.2fs)" % (time.time() - startTime))
    shards = [
        {'director':server1.name, 'replicas':[server1.name, server2.name]},
        {'director':server2.name, 'replicas':[server2.name, server1.name]}
    ]
    res = r.db(dbName).table_config(tableName).update({'shards':shards}).run(conn)
    assert res['errors'] == 0, 'Errors after splitting into two shards: %s' % repr(res)
    r.db(dbName).table_wait().run(conn)
    cluster.check()

    print("Changing the primary (%.2fs)" % (time.time() - startTime))
    shards = [
        {'director':server2.name, 'replicas':[server2.name, server1.name]},
        {'director':server1.name, 'replicas':[server1.name, server2.name]}
    ]
    assert r.db(dbName).table_config(tableName).update({'shards':shards}).run(conn)['errors'] == 0
    r.db(dbName).table_wait().run(conn)
    cluster.check()

    print("Changing it back (%.2fs)" % (time.time() - startTime))
    shards = [
        {'director':server2.name, 'replicas':[server2.name, server1.name]},
        {'director':server1.name, 'replicas':[server1.name, server2.name]}
    ]
    assert r.db(dbName).table_config(tableName).update({'shards':shards}).run(conn)['errors'] == 0

    print("Waiting for it to take effect (%.2fs)" % (time.time() - startTime))
    r.db(dbName).table_wait().run(conn)
    cluster.check()

    assert len(list(r.db('rethinkdb').table('issues').run(conn))) == 0
    
    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))
