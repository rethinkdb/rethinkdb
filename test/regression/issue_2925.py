#!/usr/bin/env python
# Copyright 2014-2015 RethinkDB, all rights reserved.

import os, multiprocessing, sys, time

sys.path.append(os.path.join(os.path.dirname(os.path.realpath(__file__)), os.path.pardir, 'common'))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()

num_rows = 30000

def populate_table(conn):
    r.db_create(dbName).run(conn)
    res = r.db(dbName).table_create(tableName).run(conn)
    assert res["tables_created"] == 1
    next_id = 0
    while next_id < num_rows:
        batch = [{'id':next_id + i, 'data': r.random(1, 100)} for i in xrange(min(1000, num_rows - next_id))]
        next_id += len(batch)
        r.db(dbName).table(tableName).insert(batch).run(conn)

def add_index(conn):
    r.db(dbName).table(tableName).index_create('data').run(conn)
    r.db(dbName).table(tableName).index_wait('data').run(conn)

def replace_rows(host, port, ready_event, start_event):
    conn = r.connect(host, port, db=dbName)
    ready_event.set()
    start_event.wait()
    try:
        for i in xrange(num_rows // 2):
            r.db(dbName).table(tableName).get(i).update({'data': r.random(1, 100)}, non_atomic=True).run(conn)
    except Exception as ex:
        if ex.message != "Connection is closed." and ex.message != "Connection is broken.":
            utils.print_with_time("ERROR: Expected to be interrupted by the connection closing, actual error: %s" % ex.message)
        return
    utils.print_with_time("ERROR: Was not interrupted while replacing entries.")

def delete_rows(host, port, ready_event, start_event):
    conn = r.connect(host, port, db=dbName)
    ready_event.set()
    start_event.wait()
    try:
        for i in xrange(num_rows // 2, num_rows):
            r.db(dbName).table(tableName).get(i).delete().run(conn)
    except Exception as ex:
        if ex.message != "Connection is closed." and ex.message != "Connection is broken.":
            utils.print_with_time("ERROR: Expected to be interrupted by the connection closing, actual error: %s" % ex.message)
        return
    utils.print_with_time("ERROR: Was not interrupted interrupted while deleting entries.")

def check_data(conn):
    utils.print_with_time("Waiting for index...")
    r.db(dbName).table(tableName).index_wait('data').run(conn)
    r.db(dbName).table(tableName).wait().run(conn)
    utils.print_with_time("Index ready, checking data...")
    pkey_count = r.db(dbName).table(tableName).count().run(conn)

    # Actually read out all of the rows from the sindexes in case of corruption
    sindex_count = r.db(dbName).table(tableName).order_by(index='data').count().run(conn)
    sindex_cursor = r.db(dbName).table(tableName).order_by(index='data').run(conn)
    rows = list(sindex_cursor)

    if len(rows) != pkey_count or pkey_count != sindex_count:
        utils.print_with_time("ERROR: inconsistent row counts between the primary and secondary indexes.")
        print("  primary - %d" % pkey_count)
        print("  secondary - %d (%d)" % (sindex_count, len(rows)))

utils.print_with_time("Spinning a cluster with one server")
with driver.Process(name='.', command_prefix=command_prefix, extra_options=serve_options) as process:
    
    utils.print_with_time("Establishing ReQL connection")
    conn = r.connect(process.host, process.driver_port, db=dbName)
    
    utils.print_with_time("Starting replace/delete processes")
    
    # Get the replace/delete processes ready ahead of time -
    # Time is critical during the sindex post-construction
    ready_events = [multiprocessing.Event(), multiprocessing.Event()]
    start_event = multiprocessing.Event()
    replace_proc = multiprocessing.Process(target=replace_rows, args=(process.host,
                                                                      process.driver_port,
                                                                      ready_events[0],
                                                                      start_event))
    delete_proc = multiprocessing.Process(target=delete_rows, args=(process.host,
                                                                    process.driver_port,
                                                                    ready_events[1],
                                                                    start_event))
    replace_proc.start()
    delete_proc.start()

    utils.print_with_time("Creating and populating table")
    populate_table(conn)

    utils.print_with_time("Creating secondary index")
    add_index(conn)
    conn.close()

    utils.print_with_time("Killing the server during a replace/delete workload")
    for event in ready_events:
        event.wait()
    start_event.set()

    # Give the queries a little time to start working - if they finish too early an error will be printed
    time.sleep(2)
    process.close()

    # Restart the process
    time.sleep(1)
    utils.print_with_time("Restarting the server")
    process.start()
    process.wait_until_ready()

    conn = r.connect(process.host, process.driver_port, db=dbName)
    time.sleep(1)
    check_data(conn)
    conn.close()

    utils.print_with_time("Cleaning up")
utils.print_with_time("Done.")
