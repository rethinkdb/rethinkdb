# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/env python
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, http_admin, scenario_common
from memcached_workload_common import MemcacheConnection
from vcoptparse import *

op = OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    print "Starting cluster..."
    processes = [
        driver.Process(cluster,
                       driver.Files(metacluster, log_path = ("create-output-%d" % (i + 1)), executable_path = executable_path, command_prefix = command_prefix),
                       executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
        for i in xrange(2)]
    for process in processes:
        process.wait_until_started_up()
    print "Creating namespace..."
    http = http_admin.ClusterAccess([("localhost", p.http_port) for p in processes])
    dc = http.add_datacenter()
    for machine_id in http.machines:
        http.move_server_to_datacenter(machine_id, dc)
    ns = http.add_namespace(protocol = "memcached", primary = dc)
    http.wait_until_blueprint_satisfied(ns)

    print "Getting distribution first time."
    distribution = http.get_distribution(ns)

    print "Inserting a bunch."
    host, port = driver.get_namespace_host(ns.port, processes)
    with MemcacheConnection(host, port) as mc:
        for i in range(10000):
            if (i + 1) % 100 == 0:
                print i + 1,
                sys.stdout.flush()
            mc.set(str(i) * 10, str(i)*20)
        print

    time.sleep(1)

    print "Getting distribution second time."
    distribution = http.get_distribution(ns)

    cluster.check_and_stop()
