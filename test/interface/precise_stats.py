#!/usr/bin/env python
# Copyright 2010-2012 RethinkDB, all rights reserved.

# This file tests the `rethinkdb.stats` admin table.
# Here, we run very particular queries and verify that the 'total' stats are exactly
# correct.  This includes point reads/writes, range reads/replaces, backfills, and
# sindex construction.

from __future__ import print_function

import sys, os, time, re, multiprocessing, random, pprint
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse, workload_runner

r = utils.import_python_driver()

db_name = 'test'
table_name = 'foo'
server_names = ['grey', 'face']

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    _, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)

    print('Spinning up %d processes...' % len(server_names))
    servers = [ ]
    for i in xrange(len(server_names)):
        info = { 'name': server_names[i] }
        info['files'] = driver.Files(metacluster, db_path='db-%d' % i,
                                     console_output='create-output-%d' % i,
                                     server_name=info['name'], command_prefix=command_prefix)
        info['process'] = driver.Process(cluster, info['files'],
                                         console_output='serve-output-%d' % i,
                                         command_prefix=command_prefix, extra_options=serve_options)
        servers.append(info)

    for server in servers:
        server['process'].wait_until_started_up()

    r.connect(servers[0]['process'].host, servers[0]['process'].driver_port).repl()

    r.db_create(db_name).run()
    r.db(db_name).table_create(table_name).run()
    tbl = r.db(db_name).table(table_name)
    table_id = tbl.config()['id'].run()

    for server in servers:
        server['id'] = r.db('rethinkdb').table('server_config') \
                        .filter(r.row['name'].eq(server['name']))[0]['id'].run()

    def get_stats(server):
        return r.db('rethinkdb').table('stats').get(['table_server', table_id, server['id']]) \
                .do({'reads':  r.row['query_engine']['read_docs_total'],
                     'writes': r.row['query_engine']['written_docs_total']}).run()

    def check_stats_internal(query, reads, writes):
        stats = get_stats(servers[0])
        delta_reads = stats['reads'] - check_stats_internal.last_reads
        delta_writes = stats['writes'] - check_stats_internal.last_writes
        if delta_reads != reads:
            print('Error in query %s: Expected %d reads but got %d' %
                  (r.errors.QueryPrinter(query).print_query(), reads, delta_reads))
        if delta_writes != writes:
            print('Error in query %s: Expected %d writes but got %d' %
                  (r.errors.QueryPrinter(query).print_query(), writes, delta_writes))
        check_stats_internal.last_reads = stats['reads']
        check_stats_internal.last_writes = stats['writes']
    check_stats_internal.last_reads = 0
    check_stats_internal.last_writes = 0

    def check_error_stats(query, reads, writes):
        try:
            query.run()
            print("Failed to error in query (%s)" % r.errors.QueryPrinter(query).print_query())
        except r.RqlError as e:
            pass
        check_stats_internal(query, reads, writes)

    def check_query_stats(query, reads, writes):
        query.run()
        check_stats_internal(query, reads, writes)

    def primary_shard(server):
        return { 'primary_replica': server['name'], 'replicas': [ server['name'] ] }

    # Pin the table to one server
    tbl.config().update({'shards': [ primary_shard(servers[0]) ]}).run()
    tbl.wait().run()

    # Create a secondary index that will be used in some of the inserts
    check_query_stats(tbl.index_create('double', r.row['id']), reads=0, writes=0)
    check_query_stats(tbl.index_create('value'), reads=0, writes=0)
    
    tbl.index_wait().run()
    
    # batch of writes
    check_query_stats(tbl.insert(r.range(100).map(lambda x: {'id': x})), reads=0, writes=100)

    # point operations
    check_query_stats(tbl.insert({'id': 100}), reads=0, writes=1)
    check_query_stats(tbl.get(100).delete(), reads=0, writes=1)
    check_query_stats(tbl.get(50), reads=1, writes=0)
    check_query_stats(tbl.get(101).replace({'id': 101}), reads=0, writes=1)
    check_query_stats(tbl.get(101).delete(), reads=0, writes=1)
    # point writes with changed rows
    check_query_stats(tbl.get(50).update({'value': 'dummy'}), reads=0, writes=1)
    check_query_stats(tbl.get(50).replace({'id': 50}), reads=0, writes=1)
    check_query_stats(tbl.get(50).update(lambda x: {'value': 'dummy'}), reads=0, writes=1)
    check_query_stats(tbl.get(50).replace(lambda x: {'id': 50}), reads=0, writes=1)
    # point writes with unchanged rows
    check_query_stats(tbl.get(50).update({}), reads=0, writes=1)
    check_query_stats(tbl.get(50).update(lambda x: {}), reads=0, writes=1)
    check_query_stats(tbl.get(50).replace({'id': 50}), reads=0, writes=1)
    check_query_stats(tbl.get(50).replace(lambda x: x), reads=0, writes=1)

    # range operations
    check_query_stats(tbl.between(20, 40), reads=20, writes=0)
    # range writes with some changed and some unchanged rows
    check_query_stats(tbl.between(20, 40).update(r.branch(r.row['id'].mod(2).eq(1), {}, {'value': 'dummy'})),
                      reads=20, writes=20)
    check_query_stats(tbl.between(20, 40).replace({'id': r.row['id']}), reads=20, writes=20)
    # range writes with unchanged rows
    check_query_stats(tbl.between(20, 40).update({}), reads=20, writes=20)
    check_query_stats(tbl.between(20, 40).replace(lambda x: x), reads=20, writes=20)

    # Operations with existence errors should still show up
    check_query_stats(tbl.get(101), reads=1, writes=0)
    check_query_stats(tbl.get(101).delete(), reads=0, writes=1)
    check_query_stats(tbl.get(101).update({}), reads=0, writes=1)
    check_query_stats(tbl.get(101).update(lambda x: {}), reads=0, writes=1)
    check_query_stats(tbl.get(101).replace(lambda x: x), reads=0, writes=1)
    check_query_stats(tbl.insert({'id': 0}), reads=0, writes=1)

    # Add a sindex, make sure that all rows are represented
    check_query_stats(r.expr([tbl.index_create('fake', r.row['id']), tbl.index_wait()]), reads=100, writes=100)

    check_query_stats(tbl.count(), reads=100, writes=0)
    check_query_stats(tbl.between(20, 40).count(), reads=20, writes=0)
    check_query_stats(tbl.between(20, 40).update(r.branch(r.row['id'].mod(2).eq(1), {}, {'value': 'dummy'})),
                      reads=20, writes=20)

    # Count on a sindex
    check_query_stats(tbl.between(r.minval, r.maxval, index='double').count(), reads=100, writes=0)
    check_query_stats(tbl.between(r.minval, r.maxval, index='value').count(), reads=10, writes=0)
    # Can't test dropping the index because the stats will disappear once it's gone

    backfiller_stats_before = get_stats(servers[0])
    backfillee_stats_before = get_stats(servers[1])

    # Shard the table into two shards (1 primary replica on each server)
    tbl.config().update({'shards': [ primary_shard(servers[0]),
                                     primary_shard(servers[1]) ]}).run()
    tbl.wait().run()
    # Manually check stats here as the number of reads/writes will be unpredictable
    backfiller_stats_after = get_stats(servers[0])
    backfillee_stats_after = get_stats(servers[1])
    backfiller_reads = backfiller_stats_after['reads'] - backfiller_stats_before['reads']
    backfiller_writes = backfiller_stats_after['writes'] - backfiller_stats_before['writes']
    backfillee_reads = backfillee_stats_after['reads'] - backfillee_stats_before['reads']
    backfillee_writes = backfillee_stats_after['writes'] - backfillee_stats_before['writes']

    if backfiller_reads > 60 or backfiller_reads < 40: # Backfiller should transfer roughly half the rows
        print("During backfill, on backfiller: Expected between 40 and 60 reads but got %d" % backfiller_reads)
    if backfillee_reads != 0: # Backfillee didn't have any rows to read - we only want writes
        print("During backfill, on backfillee: Expected 0 reads but got %d" % backfillee_reads)
    if backfiller_writes != backfiller_reads: # Backfiller should have deleted the rows it transfered
        print("During backfill, on backfiller: Expected %d writes but got %d" % (backfiller_reads, backfiller_writes))
    if backfillee_writes != backfiller_reads: # Backfillee should have written the same number of rows transferred
        print("During backfill, on backfillee: Expected %d writes but got %d" % (backfiller_reads, backfillee_writes))

    cluster.check_and_stop()

print('Done.')
