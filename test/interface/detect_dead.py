#!/usr/bin/env python
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, http_admin, scenario_common
from vcoptparse import *

op = OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster1 = driver.Cluster(metacluster)
    executable_path, command_prefix  = scenario_common.parse_mode_flags(opts)
    print "Spinning up two processes..."
    files1 = driver.Files(metacluster, executable_path = executable_path, command_prefix = command_prefix)
    proc1 = driver.Process(cluster1, files1, executable_path = executable_path, command_prefix = command_prefix)
    files2 = driver.Files(metacluster, executable_path = executable_path, command_prefix = command_prefix)
    proc2 = driver.Process(cluster1, files2, executable_path = executable_path, command_prefix = command_prefix)
    proc1.wait_until_started_up()
    proc2.wait_until_started_up()
    cluster1.check()
    access1 = http_admin.ClusterAccess([("localhost", proc1.http_port)])
    access2 = http_admin.ClusterAccess([("localhost", proc2.http_port)])
    assert len(access1.get_directory()) == len(access2.get_directory()) == 2
    print "Splitting cluster, then waiting 20s..."
    cluster2 = driver.Cluster(metacluster)
    metacluster.move_processes(cluster1, cluster2, [proc2])
    time.sleep(20)
    print "Checking that they detected the netsplit..."
    assert len(access1.get_directory()) == len(access2.get_directory()) == 1
    cluster1.check()
    cluster2.check()
    print "Joining cluster, then waiting 10s..."
    metacluster.move_processes(cluster2, cluster1, [proc2])
    time.sleep(10)
    print "Checking that they detected the resolution..."
    assert len(access1.get_directory()) == len(access2.get_directory()) == 2
    cluster1.check_and_stop()
print "Done."

