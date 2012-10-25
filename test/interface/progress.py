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
                       driver.Files(metacluster, log_path = "create-output-%d" % i,
                                    executable_path = executable_path, command_prefix = command_prefix),
                       log_path = "serve-output-%d" % i,
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
    time.sleep(10)
    host, port = driver.get_namespace_host(ns.port, processes)

    print "Performing test queries..."
    with MemcacheConnection(host, port) as mc:
        for i in range(10000):
            if (i + 1) % 100 == 0:
                print i + 1,
                sys.stdout.flush()
            mc.set(str(i) * 10, str(i)*20)
        print
    print "Done with test queries."

    print "Adding a replica."
    http.set_namespace_affinities(ns, {dc : 1})

    time.sleep(1)

    print "Checking backfill progress... ",
    progress = http.get_progress()
    for machine_id, temp1 in progress.iteritems():
        for namespace_id, temp2 in temp1.iteritems():
            for activity_id, temp3 in temp2.iteritems():
                for region, progress_val in temp3.iteritems():
                    assert(progress_val[0] != "Timeout")
                    assert(progress_val[0][0] <= progress_val[0][1])
    print "OK."

    print "Shutting down cluster."
    cluster.check_and_stop()
