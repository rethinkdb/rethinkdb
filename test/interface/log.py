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
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    print "Spinning up a process..."
    files = driver.Files(metacluster, db_path = "db", log_path = "create-output",
                         executable_path = executable_path, command_prefix = command_prefix)
    proc = driver.Process(cluster, files, log_path = "serve-output",
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
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
    time.sleep(10)
    print "Trying to access its log..."
    log = access.get_log(access.machines.keys()[0], max_length = num_entries + 100)
    print "Log is %d lines" % len(log)
    assert len(log) >= num_entries
    cluster.check_and_stop()
print "Done."

