#!/usr/bin/env python
# Copyright 2010-2012 RethinkDB, all rights reserved.

# This file tests the `rethinkdb.stats` admin table.
# The scenario works by starting a cluster of two servers and two tables. The tables are
# then sharded across the two servers (no replicas), and populated with 100 rows.
# A small read/write workload runs in the background during the entire test to ensure
# that we have stats to read.  In addition, we run with a cache-size of zero to force
# disk reads and writes.
#
# 1. Cluster is started, table populated
# 2. Gather and verify stats
# 3. Shut down the second server
# 4. Gather and verify stats - observe timeouts for the missing server
# 5. Restart the second server
# 6. Gather and verify stats
#
# Stats verification is rather weak because we can't expect specific values for many
# fields.  For most of them, we simple assert that they are greater than zero.  In
# addition, the full scan of the `stats` table in verified for internal consistency.
# That is, we make sure the tables' and servers' stats add up to the cluster stats,
# and so on.  This is not valid when getting rows from the stats table individually,
# as there will be race conditions then.

from __future__ import print_function

import sys, os, time, re, multiprocessing, random, pprint
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse, workload_runner

r = utils.import_python_driver()

db = 'test'
server_names = ['nate', 'grey']
table_names = ['foo', 'bar']

def read_write_workload(port, table, stop_event):
    conn = r.connect("localhost", port)
    ids = list(r.range(100).map(lambda x: r.uuid()).run(conn))
    r.db(db).table(table).insert([{'id': i, 'value': 0} for i in ids]).run(conn)

    # Increment this every loop so the update actually results in a write
    counter = 0
    while not stop_event.is_set():
        counter += 1
        try:
            r.db(db).table(table).get(random.choice(ids)).run(conn)
            r.db(db).table(table).insert({'id':random.choice(ids), 'value': counter},
                                         conflict='replace').run(conn)
            time.sleep(0.05)
        except r.RqlRuntimeError:
            # Ignore runtime errors and keep going until stopped
            pass

# Per-second values are floats, so do a fuzzy comparison to allow for accumulated error
def fuzzy_compare(left, right):
    return (left - right) < 1e-03

def find_rows(global_stats, pred):
    res = [ ]
    for row in global_stats:
        if pred(row['id']):
            res.append(row)
    assert len(res) != 0, "Missing stats row"
    return res

def check_sum_stat(path, iterable, expected):
    def walk_object(path, o):
        for p in path:
            o = o[p]
        return o

    total = 0.0
    for item in iterable:
        # Don't count the row if it errored - the stats are missing anyway
        if 'error' not in item:
            total += walk_object(path, item)
    if 'error' not in expected:
        assert fuzzy_compare(total, walk_object(path, expected)), \
           "Stats (%s) did not add up, expected %f, got %f" % (repr(path), total, walk_object(expected))

# Verifies that the table_server stats add up to the table stats
def check_table_stats(table_id, global_stats):
    table_row = find_rows(global_stats, lambda row_id: row_id == ['table', table_id])
    assert len(table_row) == 1
    table_row = table_row[0]

    table_server_rows = find_rows(global_stats,
                                  lambda row_id: len(row_id) == 3 and \
                                                 row_id[0] == 'table_server' and \
                                                 row_id[1] == table_id)
    check_sum_stat(['query_engine', 'read_docs_per_sec'], table_server_rows, table_row)
    check_sum_stat(['query_engine', 'written_docs_per_sec'], table_server_rows, table_row)

# Verifies that the table_server stats add up to the server stats
def check_server_stats(server_id, global_stats):
    server_row = find_rows(global_stats, lambda row_id: row_id == ['server', server_id])
    assert len(server_row) == 1
    server_row = server_row[0]

    table_server_rows = find_rows(global_stats,
                                  lambda row_id: len(row_id) == 3 and \
                                                 row_id[0] == 'table_server' and \
                                                 row_id[2] == server_id)
    check_sum_stat(['query_engine', 'read_docs_per_sec'], table_server_rows, server_row)
    check_sum_stat(['query_engine', 'written_docs_per_sec'], table_server_rows, server_row)
    check_sum_stat(['query_engine', 'read_docs_total'], table_server_rows, server_row)
    check_sum_stat(['query_engine', 'written_docs_total'], table_server_rows, server_row)

# Verifies that table and server stats add up to the cluster stats
def check_cluster_stats(global_stats):
    cluster_row = find_rows(global_stats, lambda row_id: row_id == ['cluster'])
    assert len(cluster_row) == 1
    cluster_row = cluster_row[0]

    table_rows = find_rows(global_stats,
                           lambda row_id: len(row_id) == 2 and \
                                          row_id[0] == 'table')
    check_sum_stat(['query_engine', 'read_docs_per_sec'], table_rows, cluster_row)
    check_sum_stat(['query_engine', 'written_docs_per_sec'], table_rows, cluster_row)

    server_rows = find_rows(global_stats,
                            lambda row_id: len(row_id) == 2 and \
                                           row_id[0] == 'server')
    check_sum_stat(['query_engine', 'read_docs_per_sec'], server_rows, cluster_row)
    check_sum_stat(['query_engine', 'written_docs_per_sec'], server_rows, cluster_row)
    check_sum_stat(['query_engine', 'client_connections'], server_rows, cluster_row)
    check_sum_stat(['query_engine', 'clients_active'], server_rows, cluster_row)

def get_and_check_global_stats(tables, servers, conn):
    global_stats = list(r.db('rethinkdb').table('stats').run(conn))

    check_cluster_stats(global_stats)
    for table in tables:
        check_table_stats(table['id'], global_stats)
    for server in servers:
        check_server_stats(server['id'], global_stats)

    assert len(global_stats) == 1 + len(tables) + len(servers) + (len(tables) * len(servers))
    return global_stats
    
def get_individual_stats(global_stats, conn):
    res = [ ]
    for row in global_stats:
        rerow = r.db('rethinkdb').table('stats').get(row['id']).run(conn)
        assert isinstance(rerow, dict)
        assert rerow['id'] == row['id']
        res.append(rerow)
    return res

# Global and individual stats should be in the same order
# This also assumes that the individual stats were collected after the global stats
# The only thing we know about `per_sec` stats is that they are non-zero
# For `total` stats, we can check that they only increase with time
def compare_global_and_individual_stats(global_stats, individual_stats, expected_timeouts=[]):
    assert len(global_stats) == len(individual_stats)
    for i in xrange(len(global_stats)):
        a = global_stats[i]
        b = individual_stats[i]
        assert a['id'] == b['id']
        if a['id'][0] == 'cluster':
            assert a['query_engine']['queries_per_sec'] > 0
            assert b['query_engine']['queries_per_sec'] > 0
            assert a['query_engine']['read_docs_per_sec'] > 0
            assert b['query_engine']['read_docs_per_sec'] > 0
            assert a['query_engine']['written_docs_per_sec'] > 0
            assert b['query_engine']['written_docs_per_sec'] > 0
            assert a['query_engine']['client_connections'] == b['query_engine']['client_connections'] == len(table_names) + 1
        elif a['id'][0] == 'server':
            assert a['server'] == b['server']
            if 'error' in a:
                assert 'error' in b
                assert a['error'] == b['error']
                assert a['server'] in expected_timeouts
                continue
            assert a['query_engine']['queries_per_sec'] >= 0
            assert b['query_engine']['queries_per_sec'] >= 0
            assert a['query_engine']['read_docs_per_sec'] > 0
            assert b['query_engine']['read_docs_per_sec'] > 0
            assert a['query_engine']['written_docs_per_sec'] > 0
            assert b['query_engine']['written_docs_per_sec'] > 0
            assert a['query_engine']['queries_total'] <= b['query_engine']['queries_total']
            assert a['query_engine']['read_docs_total'] <= b['query_engine']['read_docs_total']
            assert a['query_engine']['written_docs_total'] <= b['query_engine']['written_docs_total']
        elif a['id'][0] == 'table':
            assert a['db'] == b['db']
            assert a['table'] == b['table']
            assert a['query_engine']['read_docs_per_sec'] > 0
            assert b['query_engine']['read_docs_per_sec'] > 0
            assert a['query_engine']['written_docs_per_sec'] > 0
            assert b['query_engine']['written_docs_per_sec'] > 0
        elif a['id'][0] == 'table_server':
            assert a['db'] == b['db']
            assert a['table'] == b['table']
            assert a['server'] == b['server']
            if 'error' in a:
                assert 'error' in b
                assert a['error'] == b['error']
                assert a['server'] in expected_timeouts
                continue
            assert a['query_engine']['read_docs_per_sec'] > 0
            assert b['query_engine']['read_docs_per_sec'] > 0
            assert a['query_engine']['written_docs_per_sec'] > 0
            assert b['query_engine']['written_docs_per_sec'] > 0
            assert a['query_engine']['read_docs_total'] <= b['query_engine']['read_docs_total']
            assert a['query_engine']['written_docs_total'] <= b['query_engine']['written_docs_total']
            assert a['storage_engine']['disk']['read_bytes_per_sec'] > 0
            assert a['storage_engine']['disk']['written_bytes_per_sec'] > 0
            assert b['storage_engine']['disk']['read_bytes_per_sec'] > 0
            assert b['storage_engine']['disk']['written_bytes_per_sec'] > 0
            assert a['storage_engine']['disk']['read_bytes_total'] <= b['storage_engine']['disk']['read_bytes_total']
            assert a['storage_engine']['disk']['written_bytes_total'] <= b['storage_engine']['disk']['written_bytes_total']
            # even though cache size is 0, the server may use more while processing a query
            assert a['storage_engine']['cache']['in_use_bytes'] >= 0
            assert b['storage_engine']['cache']['in_use_bytes'] >= 0
            # unfortunately we can't make many assumptions about the disk space
            assert a['storage_engine']['disk']['space_usage']['data_bytes'] >= 0
            assert a['storage_engine']['disk']['space_usage']['metadata_bytes'] >= 0
            assert b['storage_engine']['disk']['space_usage']['data_bytes'] >= 0
            assert b['storage_engine']['disk']['space_usage']['metadata_bytes'] >= 0
        else:
            assert False, "Unrecognized stats row id: %s" % repr(a['id'])

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    _, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    # We use a cache size of 0 to force disk reads
    serve_options += ['--cache-size', '0']
    
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

    conn = r.connect(servers[0]['process'].host, servers[0]['process'].driver_port)
    
    print('Creating %d tables...' % len(table_names))
    stop_event = multiprocessing.Event()

    # Store uuids for each table and server for verification purposes
    r.db_create(db).run(conn)
    tables = [ ]
    for name in table_names:
        info = { 'name': name }
        r.db(db).table_create(name, shards=2, replicas=1).run(conn)
        info['db_id'] = r.db(db).config()['id'].run(conn)
        info['id'] = r.db(db).table(info['name']).config()['id'].run(conn)
        info['workload'] = multiprocessing.Process(target=read_write_workload, args=(servers[0]['process'].driver_port, name, stop_event))
        info['workload'].start()
        tables.append(info)

    for server in servers:
        server['id'] = r.db('rethinkdb').table('server_config') \
                        .filter(r.row['name'].eq(server['name']))[0]['id'].run(conn)

    # Allow some time for the workload to get the stats going
    time.sleep(1)

    try:
        # Perform table scan, get each row individually, and check the integrity of the results
        all_stats = get_and_check_global_stats(tables, servers, conn)
        also_stats = get_individual_stats(all_stats, conn)
        compare_global_and_individual_stats(all_stats, also_stats)

        # Shut down one server
        print("Killing second server...")
        servers[1]['process'].close()
        time.sleep(5)

        # Perform table scan, observe timeouts
        all_stats = get_and_check_global_stats(tables, servers, conn)
        also_stats = get_individual_stats(all_stats, conn)
        compare_global_and_individual_stats(all_stats, also_stats, expected_timeouts=[servers[1]['name']])

        # Basic test of the `_debug_stats` table
        debug_stats_0 = r.db('rethinkdb').table('_debug_stats') \
                         .get(servers[0]["id"]).run(conn)
        debug_stats_1 = r.db('rethinkdb').table('_debug_stats') \
                         .get(servers[1]["id"]).run(conn)
        assert debug_stats_0["stats"]["eventloop"]["total"] > 0
        assert "error" in debug_stats_1

        # Restart server
        print("Restarting second server...")
        servers[1]['process'] = driver.Process(cluster, servers[1]['files'],
                                               console_output='serve-output-1',
                                               command_prefix=command_prefix, extra_options=serve_options)
        servers[1]['process'].wait_until_started_up()
        time.sleep(5)

        # Perform table scan
        all_stats = get_and_check_global_stats(tables, servers, conn)
        also_stats = get_individual_stats(all_stats, conn)
        compare_global_and_individual_stats(all_stats, also_stats)
    finally:
        stop_event.set()
        for table in tables:
            table['workload'].join()

    cluster.check_and_stop()

print('Done.')
