#!/usr/bin/env python
# Copyright 2012-2016 RethinkDB, all rights reserved.

import sys, os

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

numNodes = 2

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()

utils.print_with_time("Starting cluster of %d servers" % numNodes)
with driver.Cluster(initial_servers=numNodes, output_folder='.', wait_until_ready=True, command_prefix=command_prefix, extra_options=serve_options) as cluster:
    
    server1 = cluster[0]
    server2 = cluster[1]
    conn = r.connect(host=server1.host, port=server1.driver_port)
    
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
    shards = [
        {'primary_replica':server1.name, 'replicas':[server1.name, server2.name]},
        {'primary_replica':server2.name, 'replicas':[server2.name, server1.name]}
    ]
    res = tbl.config().update({'shards':shards}).run(conn)
    assert res['errors'] == 0, 'Errors after splitting into two shards: %s' % repr(res)
    r.db(dbName).wait(wait_for="all_replicas_ready").run(conn)
    cluster.check()

    utils.print_with_time("Changing the primary replica")
    shards = [
        {'primary_replica':server2.name, 'replicas':[server2.name, server1.name]},
        {'primary_replica':server1.name, 'replicas':[server1.name, server2.name]}
    ]
    assert tbl.config().update({'shards':shards}).run(conn)['errors'] == 0
    r.db(dbName).wait(wait_for="all_replicas_ready").run(conn)
    cluster.check()

    utils.print_with_time("Changing it back")
    shards = [
        {'primary_replica':server2.name, 'replicas':[server2.name, server1.name]},
        {'primary_replica':server1.name, 'replicas':[server1.name, server2.name]}
    ]
    assert tbl.config().update({'shards':shards}).run(conn)['errors'] == 0

    utils.print_with_time("Waiting for it to take effect")
    r.db(dbName).wait(wait_for="all_replicas_ready").run(conn)
    cluster.check()
    
    res = list(r.db('rethinkdb').table('current_issues').filter(r.row["type"] != "memory_error").run(conn))
    assert len(res) == 0, 'There were unexpected issues: \n%s' % utils.RePrint.pformat(res)
    
    utils.print_with_time("Cleaning up")
utils.print_with_time("Done.")
