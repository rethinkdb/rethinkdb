#!/usr/bin/python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin, driver, scenario_common
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'drivers', 'python')))
import rethinkdb as r
from vcoptparse import *

op = OptParser()
op["timeout"] = IntFlag("--timeout", 600)
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    print "Starting cluster..."
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    files = driver.Files(metacluster, db_path = "db", log_path = "create-output", executable_path = executable_path, command_prefix = command_prefix)
    process = driver.Process(cluster, files, log_path = "serve-output",
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
    process.wait_until_started_up()

    print "Creating namespace..."
    http = http_admin.ClusterAccess([("localhost", process.http_port)])
    dc = http.add_datacenter()
    http.move_server_to_datacenter(process.files.machine_name, dc)
    db = http.add_database("test")
    ns = http.add_namespace(protocol = "rdb", primary = dc, name = "issue_607", database = db)
    http.wait_until_blueprint_satisfied(ns)
    cluster.check()
    http.check_no_issues()

    connection = r.connect("localhost", process.driver_port, db = "test")
    
    # Insert a few tens of thousands of keys
    data = []
    for i in xrange(10):
        datum = { "value": i }
        data.append(datum)

    r.table("issue_607").insert(data).run(connection)

    # Do the following for a few different types of indexes, for sanity
    sindexes = [("sin1", 123),
                ("sin2", "abc"),
                ("sin3", [2, "re", "mi", 4]),
                ("sin4", [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1])]

    for item in sindexes:
        # Add a secondary index such that all items in the table have the same secondary key
        r.table("issue_607").index_create(item[0], lambda row: item[1]).run(connection)
        cursor = r.table("issue_607").get(item[0], item[1]).run(connection)

        if len(cursor) != len(data):
            raise RuntimeError("Invalid number of results for sindex " + item[0])

    http.check_no_issues()
    cluster.check_and_stop()
