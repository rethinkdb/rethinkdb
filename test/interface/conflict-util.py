#!/usr/bin/env python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, http_admin, scenario_common
from vcoptparse import *

op = OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster1 = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    print "Spinning up two processes..."
    files1 = driver.Files(metacluster, log_path = "create-output-1",
                          executable_path = executable_path, command_prefix = command_prefix)
    proc1 = driver.Process(cluster1, files1,
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
    files2 = driver.Files(metacluster, log_path = "create-output-2",
                          executable_path = executable_path, command_prefix = command_prefix)
    proc2 = driver.Process(cluster1, files2,
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
    proc1.wait_until_started_up()
    proc2.wait_until_started_up()
    cluster1.check()

    access1 = http_admin.ClusterAccess([("localhost", proc1.http_port)])
    access2 = http_admin.ClusterAccess([("localhost", proc2.http_port)])

    access2.update_cluster_data(10)
    assert len(access1.get_directory()) == len(access2.get_directory()) == 2


    print "Hit enter to split the cluster"
    raw_input()

    print "Splitting cluster..."
    cluster2 = driver.Cluster(metacluster)
    metacluster.move_processes(cluster1, cluster2, [proc2])
    time.sleep(20)


    print "Hit enter to rejoin the cluster"
    raw_input()
    print "Joining cluster..."
    metacluster.move_processes(cluster2, cluster1, [proc2])
    cluster1.check()
    cluster2.check()
    issues = access1.get_issues()
    #assert issues[0]["type"] == "VCLOCK_CONFLICT"
    #assert len(access1.get_directory()) == len(access2.get_directory()) == 2

    time.sleep(1000000)
print "Done."

