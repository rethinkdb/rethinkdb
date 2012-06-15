#!/usr/bin/env python
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, http_admin
from vcoptparse import *

op = OptParser()
op["mode"] = StringFlag("--mode", "debug")
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    executable_path = driver.find_rethinkdb_executable(opts["mode"])
    print "Spinning up a process..."
    files = driver.Files(metacluster, db_path = "db", log_path = "create-output", executable_path = executable_path)
    proc = driver.Process(cluster, files, log_path = "serve-output", executable_path = executable_path)
    proc.wait_until_started_up()
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
    cluster.check_and_stop()
print "Done."

