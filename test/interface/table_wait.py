#!/usr/bin/env python
# Copyright 2014 RethinkDB, all rights reserved.

"""The `interface.table_wait` test checks that waiting for a table returns when the table is available for writing."""

import multiprocessing, os, sys, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

try:
    xrange
except NameError:
    xrange = range

r = utils.import_python_driver()

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

db = "test"
tables = ["table1", "table2", "table3"]
delete_table = "delete"

def check_table_states(conn, ready):
    statuses = r.expr(tables).map(r.db(db).table(r.row).status()).run(conn)
    return all(map(lambda s: (s["status"]['ready_for_writes'] == ready), statuses))

def wait_for_table_states(conn, ready):
    while not check_table_states(conn, ready=ready):
        time.sleep(0.1)

def create_tables(conn):
    r.expr(tables).for_each(r.db(db).table_create(r.row)).run(conn)
    r.db(db).table_create(delete_table).run(conn) # An extra table to be deleted during a wait
    r.db(db).table_list().for_each(r.db(db).table(r.row).insert(r.range(200).map(lambda i: {'id':i}))).run(conn)
    r.db(db).reconfigure(shards=2, replicas=2).run(conn)
    r.db(db).wait().run(conn)
    assert check_table_states(conn, ready=True), \
        "Wait after reconfigure returned before tables were ready, statuses: %s" % str(statuses)

def spawn_table_wait(port, tbl):
    def do_table_wait(port, tbl, done_event):
        conn = r.connect("localhost", port)
        try:
            if tbl is None:
                r.db(db).wait().run(conn)
            else:
                res = r.db(db).table(tbl).wait().run(conn)
                assert res["ready"] == 1
        finally:
            done_event.set()

    def do_post_write(port, tbl, start_event):
        conn = r.connect("localhost", port)
        start_event.wait()
        if tbl is None:
            r.db(db).table_list().for_each(r.db(db).table(r.row).insert({})).run(conn)
        else:
            r.db(db).table(tbl).insert({}).run(conn)

    sync_event = multiprocessing.Event()
    wait_proc = multiprocessing.Process(target=do_table_wait, args=(port, tbl, sync_event))
    write_proc = multiprocessing.Process(target=do_post_write, args=(port, tbl, sync_event))
    wait_proc.start()
    write_proc.start()
    return write_proc

utils.print_with_time("Spinning up two servers")
with driver.Cluster(initial_servers=['a', 'b'], output_folder='.', command_prefix=command_prefix, extra_options=serve_options) as cluster:
    cluster.check()
    
    proc1 = cluster[0]
    proc2 = cluster[1]
    
    utils.print_with_time("Establishing ReQL connection")

    conn = r.connect("localhost", proc1.driver_port)
    
    if db not in r.db_list().run(conn):
        utils.print_with_time("Creating db")
        r.db_create(db).run(conn)

    utils.print_with_time("Testing simple table (several times)")
    for i in xrange(5):
        res = r.db(db).table_create("simple").run(conn)
        assert res["tables_created"] == 1
        r.db(db).table("simple").reconfigure(shards=12, replicas=1).run(conn)
        r.db(db).table("simple").wait().run(conn)
        count = r.db(db).table("simple").count().run(conn)
        assert count == 0
        res = r.db(db).table_drop("simple").run(conn)
        assert res["tables_dropped"] == 1

    utils.print_with_time("Creating %d tables" % (len(tables) + 1))
    create_tables(conn)

    utils.print_with_time("Killing second server")
    proc2.close()
    wait_for_table_states(conn, ready=False)

    utils.print_with_time("Spawning waiters")
    waiter_procs = [
        spawn_table_wait(proc1.driver_port, tables[0]),
        spawn_table_wait(proc1.driver_port, tables[1]),
        spawn_table_wait(proc1.driver_port, None)   # Wait on all tables
        ]

    utils.print_with_time("Waiting on a deleted table")
    def wait_for_deleted_table(port, db, table):
        c = r.connect("localhost", port)
        try:
            r.db(db).table(table).wait().run(c)
            raise RuntimeError("`table_wait` did not error when waiting on a deleted table.")
        except r.ReqlRuntimeError as ex:
            assert ex.message == "Table `%s.%s` does not exist." % (db, table), \
                "Unexpected error when waiting for a deleted table: %s" % ex.message

    error_wait_proc = multiprocessing.Process(target=wait_for_deleted_table, args=(proc1.driver_port, db, delete_table))
    error_wait_proc.start()
    r.db(db).table_drop(delete_table).run(conn)
    error_wait_proc.join()

    utils.print_with_time("Waiting 15 seconds")
    # Wait some time to make sure the wait doesn't return early
    waiter_procs[0].join(15)
    assert all(map(lambda w: w.is_alive(), waiter_procs)), "Wait returned while a server was still down."

    utils.print_with_time("Restarting second server")
    proc2.start()
    proc2.wait_until_ready()

    utils.print_with_time("Waiting for table readiness")
    map(lambda w: w.join(), waiter_procs)
    assert check_table_states(conn, ready=True), "`wait` returned, but not all tables are ready"

    utils.print_with_time("Cleaning up")
utils.print_with_time("Done.")

