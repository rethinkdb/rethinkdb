#!/usr/bin/env python
import sys, os, time, socket, random
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, http_admin
from vcoptparse import *

def garbage(n):
    return "".join(chr(random.randint(0, 255)) for i in xrange(n))

with driver.Metacluster() as metacluster:
    print "Spinning up a process..."
    cluster = driver.Cluster(metacluster)
    proc = driver.Process(cluster, driver.Files(metacluster, db_path = "db-1"), log_path = "serve-output-1")
    time.sleep(1)
    cluster.check()
    print "Generating garbage traffic..."
    for i in xrange(30):
        print i+1,
        sys.stdout.flush()
        s = socket.socket()
        s.connect(("localhost", proc.cluster_port))
        s.send(garbage(random.randint(0, 2000)))
        time.sleep(0.1)
        s.close()
    print
    cluster.check_and_close()
print "Done."

with driver.Metacluster() as metacluster:
    print "Spinning up another process..."
    cluster = driver.Cluster(metacluster)
    proc = driver.Process(cluster, driver.Files(metacluster, db_path = "db-2"), log_path = "serve-output-2")
    time.sleep(1)
    cluster.check()
    print "Opening and holding a connection..."
    s = socket.socket()
    s.connect(("localhost", proc.cluster_port))
    print "Trying to stop cluster..."
    cluster.check_and_close()
    s.close()
print "Done."

# TODO: Corrupt actual traffic between two processes instead of generating
# complete garbage. This might be tricky.