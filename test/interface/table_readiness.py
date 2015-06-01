#!/usr/bin/env python
# Copyright 2014 RethinkDB, all rights reserved.

"""This test checks that waiting for a table at different levels of readiness returns at the right time."""

from __future__ import print_function

import threading, os, sys, time, traceback, pprint

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

r = utils.import_python_driver()

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

db = "test"
query_server = None

def create_tables(conn):
    r.db(db).table_create('single').run(conn)
    r.db(db).table_create('majority').run(conn)
    r.db(db).reconfigure(
        replicas={'primary': 1, 'replica': 2, 'nonvoting': 3},
        primary_replica_tag='primary',
        nonvoting_replica_tags=['nonvoting'],
        shards=1
        ).run(conn)
    r.db(db).table('single').config().update({'write_acks':'single'}).run(conn)
    r.db(db).table('majority').config().update({'write_acks':'majority'}).run(conn)
    r.db(db).wait(wait_for='all_replicas_ready').run(conn)

def make_expected(default=False, **kwargs):
    expected = {'ready_for_outdated_reads': default,
                # RSI(raft): Currently we don't check `ready_for_reads` because it's sort
                # of ill-defined. See GitHub issue #4320.
                # 'ready_for_reads': default,
                'ready_for_writes': default,
                'all_replicas_ready': default}
    for key,value in kwargs.items():
        assert key in expected, 'Unrecognized readiness state: %s' % str(key)
        assert isinstance(value, bool)
        expected[key] = value
    return expected

def do_async_wait(query, expected_success, error_event):
    conn = r.connect('localhost', query_server.driver_port)
    try:
        query.run(conn)
        if not expected_success:
            error_event.set()
            assert False, 'Query should have failed but did not: %s' % str(query)
    except r.RqlRuntimeError as ex:
        if expected_success:
            error_event.set()
            raise

def start_waits(expected_wait_result, error_event):
    threads = []
    for table, readinesses in expected_wait_result.items():
        for readiness, success in readinesses.items():
            threads.append(threading.Thread(target=do_async_wait,
                                            args=(r.db(db).table(table).wait(wait_for=readiness, timeout=10),
                                                  success,
                                                  error_event)))
            threads[-1].start()
    return threads

def transition_cluster(cluster, servers, files, state):
    assert all((x in servers and x in files and x in state) for x in ['primary', 'replicas'])
    assert len(servers['replicas']) == len(files['replicas'])
    assert len(servers['replicas']) == len(state['replicas'])
    assert len(servers['nvrs']) == len(files['nvrs'])
    assert len(servers['nvrs']) == len(state['nvrs'])
    assert all(x in ['up', 'down'] for x in state['replicas'])

    message_parts = []
    if state['primary'] == 'up':
        message_parts.append('primary')
    if 'up' in state['replicas']:
        count = state['replicas'].count('up')
        message_parts.append('%d replica' % count + ('s' if count > 1 else ''))
    if 'up' in state['nvrs']:
        count = state['nvrs'].count('up')
        message_parts.append('%d nonvoting replica' % count + ('s' if count > 1 else ''))
    if len(message_parts) == 0:
        message_parts.append('no replicas')
    print("Transitioning to %s up (%.2fs)" % (' and '.join(message_parts), time.time() - startTime))

    def up_down_server(server, files, new_state):
        if new_state == 'up' and not server.running:
            return driver.Process(cluster, files, console_output=True,
                                  command_prefix=command_prefix, extra_options=serve_options)
        elif new_state == 'down' and server.running:
            server.close()
        return server

    servers['primary'] = up_down_server(servers['primary'], files['primary'], state['primary'])
    for i in xrange(len(servers['replicas'])):
        servers['replicas'][i] = up_down_server(servers['replicas'][i], files['replicas'][i], state['replicas'][i])
    for i in xrange(len(servers['nvrs'])):
        servers['nvrs'][i] = up_down_server(servers['nvrs'][i], files['nvrs'][i], state['nvrs'][i])

# Waits for the query server to see the currently-running servers
# This does not wait for disconnections or for reactors to reach a stable state
def wait_for_transition(cluster):
    running_procs = [x for x in cluster[:] if x.running]
    [x.wait_until_started_up() for x in running_procs]
    uuids = [proc.uuid for proc in running_procs]

    conn = r.connect('localhost', query_server.driver_port)
    while not all(r.expr(uuids).map(r.db('rethinkdb').table('server_status').get(r.row)['status'].eq('connected')).run(conn)):
        time.sleep(0.1)

def test_wait(cluster, servers, files, states, expected_wait_result):
    # Transition through each state excluding the final state
    for state in states:
        transition_cluster(cluster, servers, files, state)
        wait_for_transition(cluster)

    print("Collecting table wait results (%.2fs)" % (time.time() - startTime))
    error_event = threading.Event()
    wait_threads = start_waits(expected_wait_result, error_event)

    [x.join() for x in wait_threads]
    if error_event.is_set():
        conn = r.connect('localhost', query_server.driver_port)
        statuses = list(r.db('rethinkdb').table('table_status').run(conn))
        assert False, 'Wait failed, table statuses: %s' % str(statuses)

print("Spinning up seven servers (%.2fs)" % (time.time() - startTime))
with driver.Cluster(initial_servers=['query', 'primary', 'r1', 'r2', 'nv1', 'nv2', 'nv3'],
                    output_folder='.', command_prefix=command_prefix,
                    extra_options=serve_options) as cluster:
    cluster.check()

    query_server = cluster[0]

    table_servers = {
        'primary': cluster[1],
        'replicas': cluster[2:4],
        'nvrs': cluster[4:] }
    table_files = {
        'primary': cluster[1].files,
        'replicas': [x.files for x in cluster[2:4]],
        'nvrs': [x.files for x in cluster[4:]] }

    print("Establishing ReQL connection (%.2fs)" % (time.time() - startTime))

    conn = r.connect("localhost", query_server.driver_port)

    # Set the server tags
    r.db('rethinkdb').table('server_config').get(table_servers['primary'].uuid) \
                                            .update({'tags':['primary']}).run(conn)
    for replica in table_servers['replicas']:
        r.db('rethinkdb').table('server_config').get(replica.uuid) \
                                                .update({'tags':['replica']}).run(conn)
    for nv in table_servers['nvrs']:
        r.db('rethinkdb').table('server_config').get(nv.uuid) \
                                                .update({'tags':['nonvoting']}).run(conn)

    if db not in r.db_list().run(conn):
        print("Creating db (%.2fs)" % (time.time() - startTime))
        r.db_create(db).run(conn)

    print("Creating 'single' and 'majority' write_ack tables (%.2fs)" % (time.time() - startTime))
    create_tables(conn)

    test_wait(cluster, table_servers, table_files,
              [{'primary': 'down', 'replicas': ['down', 'down'], 'nvrs': ['down', 'down', 'down']},
               {'primary': 'down', 'replicas': ['down', 'down'], 'nvrs': ['down', 'down', 'down']}],
              {'single': make_expected(default=False),
               'majority': make_expected(default=False)})

    test_wait(cluster, table_servers, table_files,
              [{'primary': 'down', 'replicas': ['down', 'down'], 'nvrs': ['down', 'down', 'down']},
               {'primary': 'down', 'replicas': ['up', 'down'], 'nvrs': ['down', 'down', 'down']}],
              {'single': make_expected(default=False, ready_for_outdated_reads=True),
               'majority': make_expected(default=False, ready_for_outdated_reads=True)})

    test_wait(cluster, table_servers, table_files,
              [{'primary': 'down', 'replicas': ['down', 'down'], 'nvrs': ['down', 'down', 'down']},
               {'primary': 'down', 'replicas': ['down', 'down'], 'nvrs': ['up', 'down', 'down']}],
              {'single': make_expected(default=False, ready_for_outdated_reads=True),
               'majority': make_expected(default=False, ready_for_outdated_reads=True)})

    test_wait(cluster, table_servers, table_files,
              [{'primary': 'down', 'replicas': ['down', 'down'], 'nvrs': ['down', 'down', 'down']},
               {'primary': 'up', 'replicas': ['down', 'down'], 'nvrs': ['down', 'down', 'down']}],
              {'single': make_expected(default=False, ready_for_outdated_reads=True),
               'majority': make_expected(default=False, ready_for_outdated_reads=True)})

    test_wait(cluster, table_servers, table_files,
              [{'primary': 'down', 'replicas': ['down', 'down'], 'nvrs': ['down', 'down', 'down']},
               {'primary': 'up', 'replicas': ['up', 'up'], 'nvrs': ['up', 'up', 'up']}],
              {'single': make_expected(default=True),
               'majority': make_expected(default=True)})

    test_wait(cluster, table_servers, table_files,
              [{'primary': 'up', 'replicas': ['up', 'up'], 'nvrs': ['up', 'up', 'up']}],
              {'single': make_expected(default=True),
               'majority': make_expected(default=True)})

    test_wait(cluster, table_servers, table_files,
              [{'primary': 'up', 'replicas': ['up', 'up'], 'nvrs': ['down', 'down', 'down']}],
              {'single': make_expected(default=True, all_replicas_ready=False),
               'majority': make_expected(default=True, all_replicas_ready=False)})

    test_wait(cluster, table_servers, table_files,
              [{'primary': 'up', 'replicas': ['down', 'down'], 'nvrs': ['down', 'down', 'down']}],
              {'single': make_expected(default=False, ready_for_outdated_reads=True),
               'majority': make_expected(default=False, ready_for_outdated_reads=True)})

    test_wait(cluster, table_servers, table_files,
              [{'primary': 'up', 'replicas': ['down', 'down'], 'nvrs': ['up', 'up', 'up']}],
              {'single': make_expected(default=False, ready_for_outdated_reads=True),
               'majority': make_expected(default=False, ready_for_outdated_reads=True)})

    test_wait(cluster, table_servers, table_files,
              [{'primary': 'up', 'replicas': ['up', 'down'], 'nvrs': ['down', 'down', 'down']}],
              {'single': make_expected(default=True, all_replicas_ready=False),
               'majority': make_expected(default=True, all_replicas_ready=False)})

    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))

