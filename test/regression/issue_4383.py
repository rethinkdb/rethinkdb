#!/usr/bin/env python
# Copyright 2015-2016 RethinkDB, all rights reserved.

'''Test that a backfill will resume after restarting a cluster'''

import os, pprint, sys, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
op["num_rows"] = vcoptparse.IntFlag("--num-rows", 50000)
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)
_, command_prefix, server_options = scenario_common.parse_mode_flags(opts)

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()
num_shards = 16

utils.print_with_time("Starting cluster of three servers")
with driver.Cluster(initial_servers=['source1', 'source2', 'target'], output_folder='.', console_output=True, command_prefix=command_prefix, extra_options=server_options) as cluster:
    source_a = cluster['source1']
    source_b = cluster['source2']
    target = cluster['target']
    conn = r.connect(host=source_a.host, port=source_a.driver_port)
    
    utils.print_with_time("Creating a table")
    if dbName not in r.db_list().run(conn):
        r.db_create(dbName).run(conn)
    if tableName in r.db(dbName).table_list().run(conn):
        r.db(dbName).table_drop(tableName)
    r.db("rethinkdb").table("table_config").insert({
        "name":tableName, "db": dbName,
        "shards": [{"primary_replica":"source1", "replicas":["source1", "source2"]}] * num_shards
        }).run(conn)
    tbl = r.db(dbName).table(tableName)
    tbl.wait(wait_for="all_replicas_ready").run(conn)
    
    utils.print_with_time("Inserting %d documents" % opts["num_rows"])
    chunkSize = 2000
    for startId in range(0, opts["num_rows"], chunkSize):
        endId = min(startId + chunkSize, opts["num_rows"])
        res = tbl.insert(r.range(startId, endId).map({
            "value": r.row,
            "padding": "x" * 100
            }), durability="soft").run(conn)
        assert res["inserted"] == endId - startId
        utils.print_with_time("    Progress: %d/%d" % (endId, opts["num_rows"]))
    tbl.sync().run(conn)
    
    utils.print_with_time("Beginning replication to second server")
    tbl.config().update({
        "shards": [{"primary_replica": "source1", "replicas": ["source1", "source2", "target"]}] * num_shards
        }).run(conn)
    
    utils.print_with_time("Waiting a few seconds for backfill to get going")
    deadline = time.time() + 2
    while True:
        status = tbl.status().run(conn)
        try:
            assert status["status"]["ready_for_writes"] == True, 'Table is not ready for writes:\n' + pprint.pformat(status)
            assert status["status"]["all_replicas_ready"] == False, 'All replicas incorrectly reporting ready:\n' + pprint.pformat(status)
            break
        except AssertionError:
            if time.time() > deadline:
                raise
            else:
                time.sleep(.05)
    
    utils.print_with_time("Shutting down servers")
    cluster.check_and_stop()

    utils.print_with_time("Restarting servers")
    source_a.start()
    source_b.start()
    target.start()
    conn = r.connect(host=source_a.host, port=source_a.driver_port)
    conn_target = r.connect(host=target.host, port=target.driver_port)

    utils.print_with_time("Checking that table is available for writes")
    try:
        tbl.wait(wait_for="ready_for_writes", timeout=30).run(conn)
    except r.ReqlRuntimeError, e:
        status = r.db("rethinkdb").table("_debug_table_status").nth(0).run(conn)
        pprint.pprint(status)
        raise
    try:
        tbl.wait(wait_for="ready_for_writes", timeout=3).run(conn_target)
    except r.ReqlRuntimeError, e:
        pprint.pprint(r.db("rethinkdb").table("_debug_table_status").nth(0).run(conn_target))
        raise

    utils.print_with_time("Making sure the backfill didn't end")
    status = tbl.status().run(conn)
    for shard in status['shards']:
        for server in shard['replicas']:
            if server['server'] == 'target' and server['state'] == 'backfilling':
                break # this will cause a double break, bypassing the outer else
        else:
            continue
        break
    else:
        raise AssertionError('There were no shards listed as backfilling:\n' + pprint.pformat(status))
    for job in r.db('rethinkdb').table('jobs').filter({'type':'backfill'}).run(conn):
        if job['info']['db'] == dbName and job['info']['table'] == tableName:
            break
    else:
        raise AssertionError('Did not find any job backfilling this table')
    assert not status["status"]["all_replicas_ready"], 'All replicas incorrectly reporting ready:\n' + pprint.pformat(status)

    utils.print_with_time("Cleaning up")
utils.print_with_time("Done.")
