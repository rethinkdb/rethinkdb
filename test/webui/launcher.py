#!/usr/bin/python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import sys, os, time, subprocess
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin, driver, workload_runner
from vcoptparse import *

op = OptParser()
op["num-nodes"] = IntFlag("--num-nodes", 1)
op["num-shards"] = IntFlag("--num-shards", 1)
op["mode"] = StringFlag("--mode", "debug")
op["tests"] = ManyPositionalArgs()
op["timeout"] = IntFlag("--timeout", 600)
op["images"] = StringFlag("--images", "./casper-results")
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    print "Starting cluster...",
    executable_path = driver.find_rethinkdb_executable(opts["mode"])
    processes = [driver.Process(cluster,
                                driver.Files(metacluster, db_path = "db-%d" % i, log_path = "create-output-%d" % i),
                                executable_path = executable_path, log_path = "serve-output-%d" % i)
                 for i in xrange(opts["num-nodes"])]
    for process in processes:
        process.wait_until_started_up()

    print "Cluster ready."

    #test = os.path.abspath(opts["tests"][0])
    #test = os.environ['RETHINKDB']+'/test/webui/'+opts["tests"][0].rstrip('\n')
    test = os.path.dirname(sys.argv[0])+'/'+opts["tests"][0].rstrip('\n')

    process = processes[0]

    # Build command with arguments for casperjs test
    cl = ['casperjs', 
            '--rdb-server=http://localhost:%d' % process.http_port, 
            test]

    process = subprocess.Popen(cl,stdout=subprocess.PIPE)
    for line in iter(process.stdout.readline, ""):
        print line,
