# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/python
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin, driver, workload_runner, scenario_common
from vcoptparse import *

op = OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    print "Starting cluster..."
    files = driver.Files(metacluster, log_path = "create-output",
                         executable_path = executable_path, command_prefix = command_prefix)
    process = driver.Process(cluster, files,
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
    process.wait_until_started_up()
    print "Creating namespace..."
    http = http_admin.ClusterAccess([("localhost", process.http_port)])
    dc = http.add_datacenter()
    http.move_server_to_datacenter(http.machines.keys()[0], dc)
    ns = http.add_namespace(protocol = "memcached", primary = dc)
    print "Restarting server..."
    process.check_and_stop()

    process2 = driver.Process(cluster, files, executable_path = executable_path, command_prefix = command_prefix)
    process2.wait_until_started_up()
    http2 = http_admin.ClusterAccess([("localhost", process2.http_port)])

    ns1 = http.memcached_namespaces
    ns2 = http2.memcached_namespaces
    assert(len(ns1) == 1 and len(ns2) == 1)
    uuid = ns1.keys()[0]
    assert(uuid in ns2)
    assert(ns1[uuid].name == ns2[uuid].name)
    cluster.check_and_stop()
