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
    dc = access1.add_datacenter("Fizz")
    time.sleep(2)
    access2.update_cluster_data()
    assert len(access1.get_directory()) == len(access2.get_directory()) == 2
    print "Splitting cluster, then waiting 20s..."
    cluster2 = driver.Cluster(metacluster)
    metacluster.move_processes(cluster1, cluster2, [proc2])
    time.sleep(20)

    print "Conflicting datacenter name..."
    access1.rename(dc, "Buzz")
    access2.rename(access2.find_datacenter(dc.uuid), "Fizz")

    print "Joining cluster, then waiting 10s..."
    metacluster.move_processes(cluster2, cluster1, [proc2])
    time.sleep(10)
    cluster1.check()
    cluster2.check()
    issues = access1.get_issues()
    assert issues[0]["type"] == "VCLOCK_CONFLICT"
    assert len(access1.get_directory()) == len(access2.get_directory()) == 2
    cluster1.check_and_close()
print "Done."

