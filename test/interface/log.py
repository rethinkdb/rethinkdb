#!/usr/bin/env python
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, http_admin
from vcoptparse import *

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    print "Spinning up a process..."
    proc = driver.Process(cluster, driver.Files(metacluster))
    time.sleep(1)
    cluster.check()
    print "Trying to access its log..."
    access = http_admin.ClusterAccess([("localhost", proc.http_port)])
    log = access.get_log(access.machines.keys()[0])
    assert len(log) > 0
    print "Log is %d lines" % log.count("\n")
    cluster.check_and_stop()
print "Done."

