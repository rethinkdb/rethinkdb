#!/usr/bin/python
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin, driver, workload_runner, scenario_common
from vcoptparse import *

op = OptParser()
op["workload"] = PositionalArg()
op["timeout"] = IntFlag("--timeout", 600)
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    print "Starting cluster..."
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    process1 = driver.Process(cluster, driver.Files(metacluster, db_path = "db-1", executable_path = executable_path, command_prefix = command_prefix), log_path = "serve-output-1",
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
    process2 = driver.Process(cluster, driver.Files(metacluster, db_path = "db-2", executable_path = executable_path, command_prefix = command_prefix), log_path = "serve-output-2",
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
    process1.wait_until_started_up()
    process2.wait_until_started_up()

    print "Creating namespace..."
    http = http_admin.ClusterAccess([("localhost", p.http_port) for p in [process1, process2]])
    dc = http.add_datacenter()
    http.move_server_to_datacenter(process1.files.machine_name, dc)
    http.move_server_to_datacenter(process2.files.machine_name, dc)
    ns = http.add_namespace(protocol = "memcached", primary = dc, affinities = {dc: 1})
    http.do_query("POST", "/ajax/semilattice/memcached_namespaces/%s/primary_pinnings" % ns.uuid,
        {"[\"\",null]": http.find_machine(process1.files.machine_name).uuid})
    http.wait_until_blueprint_satisfied(ns)
    cluster.check()
    http.check_no_issues()

    workload_ports = scenario_common.get_workload_ports(ns.port, [process1, process2])
    workload_runner.run(opts["workload"], workload_ports, opts["timeout"])

    print "Changing the primary..."
    http.do_query("POST", "/ajax/semilattice/memcached_namespaces/%s/primary_pinnings" % ns.uuid,
        {"[\"\",null]": http.find_machine(process2.files.machine_name).uuid})

    time.sleep(1)

    print "Changing it back..."
    http.do_query("POST", "/ajax/semilattice/memcached_namespaces/%s/primary_pinnings" % ns.uuid,
        {"[\"\",null]": http.find_machine(process1.files.machine_name).uuid})

    print "Waiting for it to take effect..."
    http.wait_until_blueprint_satisfied(ns)

    http.check_no_issues()
    cluster.check_and_stop()
