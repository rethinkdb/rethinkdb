#!/usr/bin/env python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import sys, os, time, collections
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, os.path.pardir, 'build', 'packages', 'python')))
import rethinkdb as r
import driver, workload_runner, scenario_common, rdb_workload_common
from vcoptparse import *

op = OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
op["workload1"] = PositionalArg()
op["workload2"] = PositionalArg()
op["timeout"] = IntFlag("--timeout", 600)
opts = op.parse(sys.argv)

TableShim = collections.namedtuple('TableShim', ['name'])

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    print "Starting cluster..."
    files = driver.Files(metacluster, log_path = "create-output",
                         executable_path = executable_path, command_prefix = command_prefix)
    process = driver.Process(cluster, files,
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
    process.wait_until_started_up()
    print "Creating table..."
    with r.connect('localhost', process.driver_port) as conn:
        r.db_create('test').run(conn)
        r.db('test').table_create('restart').run(conn)
    ns = TableShim(name='restart')
    workload_ports = scenario_common.get_workload_ports(ns, [process])
    workload_runner.run(opts["workload1"], workload_ports, opts["timeout"])
    print "Restarting server..."
    process.check_and_stop()
    process2 = driver.Process(cluster, files,
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
    process2.wait_until_started_up()
    cluster.check()
    rdb_workload_common.wait_for_table(host="localhost", port=process2.driver_port, table=ns.name)
    workload_ports2 = scenario_common.get_workload_ports(ns, [process2])
    workload_runner.run(opts["workload2"], workload_ports2, opts["timeout"])
    print "Shutting down..."
    cluster.check_and_stop()
