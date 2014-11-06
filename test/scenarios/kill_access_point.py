#!/usr/bin/env python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin, driver, workload_runner, scenario_common
from vcoptparse import *

op = OptParser()
op["workload"] = PositionalArg()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    print "Starting cluster..."
    cluster = driver.Cluster(metacluster)
    _, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    database_server_files = driver.Files(metacluster, db_path="db-database", console_output="create-db-database-output", command_prefix=command_prefix)
    database_server = driver.Process(cluster, database_server_files, console_output="serve-output-database", command_prefix=command_prefix, extra_options=serve_options)
    access_server_files = driver.Files(metacluster, db_path="db-access", console_output="create-db-access-output", command_prefix=command_prefix)
    access_server = driver.Process(cluster, access_server_files, console_output="serve-output-access", command_prefix=command_prefix, extra_options=serve_options)
    database_server.wait_until_started_up
    access_server.wait_until_started_up()

    print "Creating table..."
    http = http_admin.ClusterAccess([("localhost", database_server.http_port)])
    database_dc = http.add_datacenter()
    http.move_server_to_datacenter(database_server.files.server_name, database_dc)
    other_dc = http.add_datacenter()
    http.move_server_to_datacenter(access_server.files.server_name, other_dc)
    ns = scenario_common.prepare_table_for_workload(http, primary=database_dc)
    http.wait_until_blueprint_satisfied(ns)
    cluster.check()
    http.check_no_issues()

    workload_ports = scenario_common.get_workload_ports(ns, [access_server])
    with workload_runner.ContinuousWorkload(opts["workload"], workload_ports) as workload:
        workload.start()
        print "Running workload for 10 seconds..."
        time.sleep(10)
        cluster.check()
        http.check_no_issues()
        print "Killing the access server..."
        access_server.close()
        # Don't bother stopping the workload, just exit and it will get killed

    http.declare_server_dead(access_server.files.server_name)
    http.check_no_issues()
    database_server.check()
    print "The other server dosn't seem to have crashed."
