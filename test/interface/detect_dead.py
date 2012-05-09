#!/usr/bin/env python
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, http_admin
from vcoptparse import *

with driver.Metacluster() as metacluster:
    cluster1 = driver.Cluster(metacluster)
    print "Spinning up two processes..."
    proc1 = driver.Process(cluster1, driver.Files(metacluster))
    proc2 = driver.Process(cluster1, driver.Files(metacluster))
    time.sleep(1)
    cluster1.check()
    access1 = http_admin.ClusterAccess([("localhost", proc1.http_port)])
    access2 = http_admin.ClusterAccess([("localhost", proc2.http_port)])
    assert len(access1.get_directory()) == len(access2.get_directory()) == 2
    print "Splitting cluster, then waiting 10s..."
    cluster2 = driver.Cluster(metacluster)
    metacluster.move_processes(cluster1, cluster2, [proc2])
    time.sleep(10)
    print "Checking that they detected the netsplit..."
    assert len(access1.get_directory()) == len(access2.get_directory()) == 1
    cluster1.check()
    cluster2.check()
    print "Joining cluster, then waiting 10s..."
    metacluster.move_processes(cluster2, cluster1, [proc2])
    time.sleep(10)
    print "Checking that they detected the resolution..."
    assert len(access1.get_directory()) == len(access2.get_directory()) == 2
    cluster1.check_and_close()
print "Done."

