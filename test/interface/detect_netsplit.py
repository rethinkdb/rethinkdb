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
    
    serverAOutput = list(r.db('rethinkdb').table('server_status').pluck('id', 'status', 'name').run(connA))
    serverBOutput = list(r.db('rethinkdb').table('server_status').pluck('id', 'status', 'name').run(connB))

    assert serverAOutput == serverBOutput, 'Output did not match:\n%s\n\tvs.\n%s' % (repr(serverAOutput), repr(serverBOutput))
    assert all([x['status'] == 'connected' for x in serverAOutput]), 'One of the servers was not online: %s' % repr(serverAOutput)
    assert sorted([serverA.uuid, serverB.uuid]) == sorted([x['id'] for x in serverAOutput])
    
    print("Splitting cluster (%.2fs)" % (time.time() - startTime))
    
    cluster2 = driver.Cluster(metacluster)
    metacluster.move_processes(cluster1, cluster2, [serverB])
    
    print("Watching up to 20 seconds to see that they detected the netsplit (%.2fs)" % (time.time() - startTime))
    
    deadline = time.time() + 20
    query = r.db('rethinkdb').table('server_status')['status'].count('disconnected').eq(1)
    while time.time() < deadline:
        if all([query.run(connA), query.run(connB)]):
            break
        time.sleep(.1)
    else:
        assert False, 'Servers did not detect the netsplit after 20 seconds'
    
    cluster1.check()
    cluster2.check()
    
    print("Checking that both servers see appropriate issues (%.2fs)" % (time.time() - startTime))
    
    for conn, name, me, him in [(connA, 'A', serverA, serverB), (connB, 'B', serverB, serverA)]:
        deadline = time.time() + 5
        lastAssertion = None
        while time.time() < deadline: # sometimes rethinkdb.current_issues takes a moment to get in sync
            try:
                issues = list(r.db('rethinkdb').table('current_issues').run(conn))
                assert len(issues) == 1, 'There was not exactly one issue visible to server%s: %s' % (name, repr(issues))
                assert issues[0]['type'] == 'server_disconnected', 'The issue for server%s did not have type "server_disconnected": %s' % (name, repr(issues))
                assert issues[0]['info']['disconnected_server'] == him.name, 'The issue for server%s had the wrong info in "info.disconnected_server": %s' % (name, repr(issues))
                assert issues[0]['info']['reporting_servers'] == [me.name], 'The issue for server%s had the wrong info in "info.reporting_servers": %s' % (name, repr(issues))
                break
            except AssertionError as e:
                lastAssertion = e
                time.sleep(.1)
        else:
            raise lastAssertion
    
    print("Joining cluster (%.2fs)" % (time.time() - startTime))
    
    metacluster.move_processes(cluster2, cluster1, [serverB])
    
    print("Watching up to 10 seconds to see that they detected the resolution (%.2fs)" % (time.time() - startTime))
    
    deadline = time.time() + 10
    query = r.db('rethinkdb').table('server_status')['status'].count('connected').eq(2)
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
