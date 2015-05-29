#!/usr/bin/env python
# Copyright 2010-2015 RethinkDB, all rights reserved.

from __future__ import print_function

import os, sys, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

r = utils.import_python_driver()

with driver.Metacluster() as metacluster:
    cluster1 = driver.Cluster(metacluster)
    
    print("Spinning up two processes (%.2fs)" % (time.time() - startTime))
    
    serverA = driver.Process(cluster1, console_output=True, command_prefix=command_prefix, extra_options=serve_options)
    serverB = driver.Process(cluster1, console_output=True, command_prefix=command_prefix, extra_options=serve_options)
    cluster1.wait_until_ready()
    cluster1.check()
    
    print("Establishing ReQL connections (%.2fs)" % (time.time() - startTime))
    
    connA = r.connect(host=serverA.host, port=serverA.driver_port)
    connB = r.connect(host=serverB.host, port=serverB.driver_port)
    
    print("Checking that both servers see each other (%.2fs)" % (time.time() - startTime))
    
    serverAOutput = list(r.db('rethinkdb').table('server_status').pluck('id', 'name').run(connA))
    serverBOutput = list(r.db('rethinkdb').table('server_status').pluck('id', 'name').run(connB))

    assert serverAOutput == serverBOutput, 'Output did not match:\n%s\n\tvs.\n%s' % (repr(serverAOutput), repr(serverBOutput))
    assert sorted([serverA.uuid, serverB.uuid]) == sorted([x['id'] for x in serverAOutput])
    
    print("Splitting cluster (%.2fs)" % (time.time() - startTime))
    
    cluster2 = driver.Cluster(metacluster)
    metacluster.move_processes(cluster1, cluster2, [serverB])
    
    print("Watching up to 20 seconds to see that they detected the netsplit (%.2fs)" % (time.time() - startTime))
    
    deadline = time.time() + 20
    query = r.db('rethinkdb').table('server_status').count().eq(1)
    while time.time() < deadline:
        if all([query.run(connA), query.run(connB)]):
            break
        time.sleep(.1)
    else:
        assert False, 'Servers did not detect the netsplit after 20 seconds'
    
    cluster1.check()
    cluster2.check()
    
    print("Joining cluster (%.2fs)" % (time.time() - startTime))
    
    metacluster.move_processes(cluster2, cluster1, [serverB])
    
    print("Watching up to 10 seconds to see that they detected the resolution (%.2fs)" % (time.time() - startTime))
    
    deadline = time.time() + 10
    query = r.db('rethinkdb').table('server_status').count().eq(2)
    while time.time() < deadline:
        if all([query.run(connA), query.run(connB)]):
            break
        time.sleep(.1)
    else:
        assert False, 'Servers did not detect the netsplit resolution after 10 seconds'
    
    cluster1.check()
    cluster2.check()
    
    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done (%.2fs)" % (time.time() - startTime))
