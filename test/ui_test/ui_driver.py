# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/python
import sys, os, time, subprocess
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin, driver, workload_runner
from vcoptparse import *
from termcolor import colored, cprint

op = OptParser()
op["num-nodes"] = IntFlag("--num-nodes", 3)
op["num-shards"] = IntFlag("--num-shards", 2)
op["mode"] = StringFlag("--mode", "debug")
op["tests"] = ManyPositionalArgs()
op["timeout"] = IntFlag("--timeout", 600)
op["images"] = StringFlag("--images", "./casper-results")
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    print "Starting cluster..."
    executable_path = driver.find_rethinkdb_executable(opts["mode"])
    processes = [driver.Process(cluster,
                                driver.Files(metacluster, db_path = "db-%d" % i, log_path = "create-output-%d" % i),
                                executable_path = executable_path, log_path = "serve-output-%d" % i)
                 for i in xrange(opts["num-nodes"])]
    for process in processes:
        process.wait_until_started_up()

    tests = [os.path.abspath(test) for test in opts["tests"]]

    process = processes[0]
    for test in tests:
        import socket; print socket.gethostname()
        # Build command with arguments for casperjs test
        cl = ['casperjs', 
                '--rdb-server=http://localhost:%d' % process.http_port, 
                test]

        test_name = os.path.basename(os.path.splitext(test)[0])
        image_dir = os.path.abspath(os.path.join(opts["images"],test_name))
        cl.extend(['--images='+image_dir])

        # Execute casperjs and pretty-print its output
        os.chdir(os.path.dirname(test))
        print cl
        process = subprocess.Popen(cl,stdout=subprocess.PIPE)
        stdout = process.stdout.readlines()
        #for i, line in enumerate(stdout):
        #    cprint('[%s]' % test_name, attrs=['bold'], end=' ')
        #    print line.rstrip('\n')
        print stdout
        print '\033[0m'
        print 'hello'
