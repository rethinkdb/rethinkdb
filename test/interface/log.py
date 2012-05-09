#!/usr/bin/env python
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, http_admin
from vcoptparse import *

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    print "Spinning up a process..."
    proc = driver.Process(cluster, driver.Files(metacluster, db_path = "db", log_path = "create-output"), log_path = "serve-output")
    time.sleep(1)
    cluster.check()
    print "Generating a bunch of log traffic..."
    access = http_admin.ClusterAccess([("localhost", proc.http_port)])
    datacenter = access.add_datacenter()
    num_entries = 100
    for i in xrange(num_entries):
        print i,
        sys.stdout.flush()
        access.rename(datacenter, str(i))
    print
    time.sleep(2)
    print "Trying to access its log..."
    log = access.get_log(access.machines.keys()[0], max_length = num_entries + 100)
    print "Log is %d lines" % len(log)
    assert len(log) >= num_entries
    cluster.check_and_close()
print "Done."

