#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.
import sys, os, time, threading, traceback
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin, driver, scenario_common, utils
from vcoptparse import *
r = utils.import_python_driver()

op = OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    print "Starting cluster..."
    cluster = driver.Cluster(metacluster)
    _, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    files1 = driver.Files(metacluster, db_path="db-1", console_output="create-output-1", command_prefix=command_prefix)
    files2 = driver.Files(metacluster, db_path="db-2", console_output="create-output-2", command_prefix=command_prefix)
    process1 = driver.Process(cluster, files1, console_output="serve-output-1", command_prefix=command_prefix, extra_options=serve_options)
    process2 = driver.Process(cluster, files2, console_output="serve-output-2", command_prefix=command_prefix, extra_options=serve_options)
    process1.wait_until_started_up()
    process2.wait_until_started_up()

    print "Creating namespace..."
    http = http_admin.ClusterAccess([("localhost", p.http_port) for p in [process1, process2]])
    dc = http.add_datacenter()
    http.move_server_to_datacenter(process1.files.machine_name, dc)
    http.move_server_to_datacenter(process2.files.machine_name, dc)
    db = http.add_database()
    ns = http.add_table(primary=dc, affinities={dc: 1}, ack_expectations={dc: 2}, database=db.name)
    http.wait_until_blueprint_satisfied(ns)
    cluster.check()
    http.check_no_issues();

    conn1 = r.connect("localhost", process1.driver_port)
    stopping = False
    def other_thread():
        try:
            while not stopping:
                r.db_create("db1").run(conn1)
                r.db_drop("db1").run(conn1)
        except Exception, e:
            traceback.print_exc(e)
            print "Aborting because of error in side thread"
            sys.exit(1)
    thr = threading.Thread(target = other_thread)
    thr.start()

    conn2 = r.connect("localhost", process2.driver_port)
    for i in xrange(1000):
        r.db_create("db2").run(conn2)
        r.db_drop("db2").run(conn2)
        http.check_no_issues()

    stopping = True
    thr.join()

