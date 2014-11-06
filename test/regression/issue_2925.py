#!/usr/bin/env python
# Copyright 2014 RethinkDB, all rights reserved.

from __future__ import print_function

import os, multiprocessing, sys, time

sys.path.append(os.path.join(os.path.dirname(os.path.realpath(__file__)), os.path.pardir, 'common'))
import driver, http_admin, utils

r = utils.import_python_driver()

table_name = 'regression_2925'
db_name = 'test'
num_rows = 30000

def populate_table(conn):
    r.db_create(db_name).run(conn)
    r.table_create(table_name).run(conn)
    next_id = 0
    while next_id < num_rows:
        batch = [{'id':next_id + i, 'data': r.random(1, 100)} for i in xrange(min(1000, num_rows - next_id))]
        next_id += len(batch)
        r.table(table_name).insert(batch).run(conn)

def add_index(conn):
    r.table(table_name).index_create('data').run(conn)

def replace_rows(host, port, ready_event, start_event):
    conn = r.connect(host, port, db=db_name)
    ready_event.set()
    start_event.wait()
    try:
        for i in xrange(num_rows // 2):
            r.table(table_name).get(i).update({'data': r.random(1, 100)}, non_atomic=True).run(conn)
    except Exception as ex:
        if ex.message != "Connection is closed." and ex.message != "Connection is broken.":
            print("ERROR: Expected to be interrupted by the connection closing, actual error: %s" % ex.message)
        return
    print("ERROR: Was not interrupted while replacing entries.")

def delete_rows(host, port, ready_event, start_event):
    conn = r.connect(host, port, db=db_name)
    ready_event.set()
    start_event.wait()
    try:
        for i in xrange(num_rows // 2, num_rows):
            r.table(table_name).get(i).delete().run(conn)
    except Exception as ex:
        if ex.message != "Connection is closed." and ex.message != "Connection is broken.":
            print("ERROR: Expected to be interrupted by the connection closing, actual error: %s" % ex.message)
        return
    print("ERROR: Was not interrupted interrupted while deleting entries.")

def check_data(conn):
    print("Waiting for index...")
    r.table(table_name).index_wait('data').run(conn)
    print("Index ready, checking data...")
    pkey_count = r.table(table_name).count().run(conn)

    # Actually read out all of the rows from the sindexes in case of corruption
    sindex_count = r.table(table_name).order_by(index='data').count().run(conn)
    sindex_cursor = r.table(table_name).order_by(index='data').run(conn)
    rows = list(sindex_cursor)

    if len(rows) != pkey_count or pkey_count != sindex_count:
        print("ERROR: inconsistent row counts between the primary key and secondary index.")
        print("  primary - %d" % pkey_count)
        print("  secondary - %d (%d)" % (sindex_count, len(rows)))

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    print("Starting server...")
    files = driver.Files(metacluster, db_path="db", console_output="create-output")
    process = driver.Process(cluster, files, console_output="serve-output")

    process.wait_until_started_up()

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

    conn = r.connect(process.host, process.driver_port, db=db_name)

    print("Creating and populating table...")
    populate_table(conn)

    print("Creating secondary index...")
    add_index(conn)
    conn.close()

    print("Killing the server during a replace/delete workload...")
    for event in ready_events:
        event.wait()
    start_event.set()

    # Give the queries a little time to start working - if they finish too early an error will be printed
    time.sleep(2)
    process.close()

    # Restart the process
    time.sleep(1)
    print("Restarting the server...")
    process = driver.Process(cluster, files, console_output="serve-output")
    process.wait_until_started_up()

    conn = r.connect(process.host, process.driver_port, db=db_name)
    time.sleep(1)
    check_data(conn)
    conn.close()

    cluster.check_and_stop()
