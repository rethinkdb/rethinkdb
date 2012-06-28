#!/usr/bin/env python
import sys, os, time, urllib2
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, http_admin, scenario_common
from vcoptparse import *

op = OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix  = scenario_common.parse_mode_flags(opts)
    print "Spinning up a process..."
    files = driver.Files(metacluster, db_path = "db", log_path = "create-output", executable_path = executable_path, command_prefix = command_prefix)
    proc = driver.Process(cluster, files, log_path = "serve-output", executable_path = executable_path, command_prefix = command_prefix)
    proc.wait_until_started_up()
    cluster.check()
    access = http_admin.ClusterAccess([("localhost", proc.http_port)])
    print "Getting root"
    assert('html' in access.do_query_plaintext("GET", "/"))
    print "Getting invalid page"
    try:
        access.do_query_plaintext("GET", "/foobar")
        assert(False and "Invalid page raises error code")
    except Exception, e:
        assert(e.status == 403)
    time.sleep(2)
    print "Trying to access its log..."
    log = access.get_log(access.machines.keys()[0], max_length = 100)
    print "Log is %d lines" % len(log)
    assert any('nonwhitelisted' in entry['message'] for entry in log)
    cluster.check_and_stop()
print "Done."

