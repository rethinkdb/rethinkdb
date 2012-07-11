#!/usr/bin/env python
import sys, os, time, socket, random
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, http_admin, scenario_common
from vcoptparse import *

op = OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

def garbage(n):
    return "".join(chr(random.randint(0, 255)) for i in xrange(n))

with driver.Metacluster() as metacluster:
    print "Spinning up a process..."
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    files = driver.Files(metacluster, db_path = "db-1", executable_path = executable_path, command_prefix = command_prefix)
    proc = driver.Process(cluster, files, log_path = "serve-output-1",
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
    proc.wait_until_started_up()
    cluster.check()
    print "Generating garbage traffic..."
    for i in xrange(30):
        print i+1,
        sys.stdout.flush()
        s = socket.socket()
        s.connect(("localhost", proc.cluster_port))
        s.send(garbage(random.randint(0, 500)))
        time.sleep(3)
        s.close()
        cluster.check()
    print
    cluster.check_and_stop()
print "Done."

with driver.Metacluster() as metacluster:
    print "Spinning up another process..."
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix  = scenario_common.parse_mode_flags(opts)
    files = driver.Files(metacluster, db_path = "db-2", executable_path = executable_path, command_prefix = command_prefix)
    proc = driver.Process(cluster, files, log_path = "serve-output-2", executable_path = executable_path, command_prefix = command_prefix)
    proc.wait_until_started_up()
    cluster.check()
    print "Opening and holding a connection..."
    s = socket.socket()
    s.connect(("localhost", proc.cluster_port))
    print "Trying to stop cluster..."
    cluster.check_and_stop()
    s.close()
print "Done."

# TODO: Corrupt actual traffic between two processes instead of generating
# complete garbage. This might be tricky.
