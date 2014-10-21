#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.
import sys, os, time, traceback
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils
from vcoptparse import *
r = utils.import_python_driver()

"""The `interface.table_config` test checks that the special `rethinkdb.table_config` and
`rethinkdb.table_status` tables behave as expected."""

op = OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

db = "test"
tables = ["table1", "table2", "table3"]

def create_tables(conn):
    r.db_create(db).run(conn)
    r.expr(tables).for_each(r.db(db).table_create(r.row)).run(conn)
    print "statuses: %s" % str(r.db(db).table_status(r.args(tables)).run(conn))
    r.expr(tables).for_each(r.db(db).table(r.row).insert(r.range(200).map(lambda i: {'id':i}))).run(conn)
    r.db(db).table_list().for_each(r.db(db).table(r.row).reconfigure(2, 2)).run(conn)

def check_table_states(conn, ready):
    statuses = r.db(db).table_config(r.args(tables)).run(conn)
    return all(map(lambda s: (s['ready_for_writes'] == ready), statuses))

def wait_for_table_states(conn, ready):
    while not check_table_states(conn, ready=ready):
        time.sleep(0.1)

def spawn_table_wait(port, tbls):
    def do_table_wait(port, tbls, done_event):
        conn = r.connect("localhost", port)
        try:
            r.db(db).table_wait(r.args(tbls)).run(conn)
        finally:
            done_event.set()

    def do_post_write(port, tbls, start_event):
        conn = r.connect("localhost", port)
        start_event.wait()
        r.expr(tbls).foreach(r.db(db).table(r.row).insert({})).run(conn)

    sync_event = multiprocessing.Event()
    wait_proc = multiprocessing.Process(target=do_table_wait, args=(port, tbls, sync_event))
    write_proc = multiprocessing.Process(target=do_port_write, args=(port, tbls, sync_event))
    wait_proc.start()
    write_proc.start()
    return wait_proc

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    print "Spinning up two processes..."
    files1 = driver.Files(metacluster, log_path = "create-output-1", machine_name = "a",
                          executable_path = executable_path, command_prefix = command_prefix)
    proc1 = driver.Process(cluster, files1, log_path = "serve-output-1",
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
    files2 = driver.Files(metacluster, log_path = "create-output-2", machine_name = "b",
                          executable_path = executable_path, command_prefix = command_prefix)
    proc2 = driver.Process(cluster, files2, log_path = "serve-output-2",
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
    proc1.wait_until_started_up()
    proc2.wait_until_started_up()
    cluster.check()

    conn = r.connect("localhost", proc1.driver_port)

    create_tables(conn)

    print "Killing second server..."
    proc2.close()

    wait_for_table_states(conn, ready=False)
    waiter_procs = [ spawn_table_wait(proc1.driver_port, [tables[0]]),            # Wait for one table
                     spawn_table_wait(proc1.driver_port, [tables[1], tables[2]]), # Wait for two tables
                     spawn_table_wait(proc1.driver_port, []) ]                    # Wait for all tables

    print "Restarting second server..."
    proc2 = driver.Process(cluster, files2, log_path = "serve-output-2",
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)

    map(lambda w: w.join(), waiter_procs)
    if not check_table_states(conn, ready=True):
        print "`table_wait` returned, but not all tables are ready"

    cluster.check_and_stop()
print "Done."

