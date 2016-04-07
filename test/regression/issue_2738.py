#!/usr/bin/env python
# Copyright 2010-2016 RethinkDB, all rights reserved.

from __future__ import print_function

import os, sys, time, threading, traceback

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

r = utils.import_python_driver()

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

numNodes = 2

print("Starting cluster of %d servers (%.2fs)" % (numNodes, time.time() - startTime))
with driver.Cluster(initial_servers=numNodes, output_folder='.', wait_until_ready=True, command_prefix=command_prefix, extra_options=serve_options) as cluster:
    
    server1 = cluster[0]
    server2 = cluster[1]
    
    print("Starting creation/destruction of db1 on server1 (%.2fs)" % (time.time() - startTime))

    conn1 = r.connect("localhost", server1.driver_port)
    stopping = False
    def other_thread():
        try:
            while not stopping:
                r.db_create("db1").run(conn1)
                r.db_drop("db1").run(conn1)
        except Exception, e:
            traceback.print_exc(e)
            sys.exit("Aborting because of error in side thread")
    thr = threading.Thread(target=other_thread)
    thr.start()
    
    print("Starting creation/destruction of db2 on server2 (%.2fs)" % (time.time() - startTime))
    
    conn2 = r.connect("localhost", server2.driver_port)
    for i in xrange(1, 501):
        r.db_create("db2").run(conn2)
        r.db_drop("db2").run(conn2)
        issues = list(r.db('rethinkdb').table('current_issues').filter(r.row["type"] != "memory_error").run(conn2))
        assert len(issues) == 0, 'Issues detected during testing: %s' % issues
        if i % 50 == 0 or i == 1:
            print(str(i), end='.. ')
            sys.stdout.flush()
    
    print("\nTesting completed (%.2fs)" % (time.time() - startTime))

    stopping = True
    thr.join()
    
    # -- ending
    
    print("Cleaning up (%.2fs)" % (time.time() - startTime))

print("Done. (%.2fs)" % (time.time() - startTime))
