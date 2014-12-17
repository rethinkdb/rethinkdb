#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

'''Start two servers, with data on only one kill and remove the second and verify that the primary continues'''

import sys, os, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse, workload_runner

op = vcoptparse.OptParser()
op["workload"] = vcoptparse.PositionalArg()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)

numNodes = 2

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()

print("Starting cluster of %d servers (%.2fs)" % (numNodes, time.time() - startTime))
with driver.Cluster(initial_servers=numNodes, output_folder='.', wait_until_ready=True, command_prefix=command_prefix, extra_options=serve_options) as cluster:

    print("Establishing ReQL Connection (%.2fs)" % (time.time() - startTime))
    database_server = cluster[0]
    access_server = cluster[1]
    conn = r.connect(host=database_server.host, port=database_server.driver_port)

    print("Creating db/table %s/%s (%.2fs)" % (dbName, tableName, time.time() - startTime))

    if dbName not in r.db_list().run(conn):
        r.db_create(dbName).run(conn)

    if tableName in r.db(dbName).table_list().run(conn):
        r.db(dbName).table_drop(tableName).run(conn)
    r.db(dbName).table_create(tableName).run(conn)

    print("Pinning database to only the first server (%.2fs)" % (time.time() - startTime))

    assert r.db(dbName).table_config(tableName).update({'shards':[{'primary_replica':database_server.name, 'replicas':[database_server.name]}]}).run(conn)['errors'] == 0
    r.db(dbName).table_wait().run(conn)
    cluster.check()
    assert [] == list(r.db('rethinkdb').table('issues').run(conn))

    sys.stderr.write('before workoad: %s\n' % (repr(list(r.db(dbName).table_config(tableName).run(conn)))))

    print("Starting workload (%.2fs)" % (time.time() - startTime))

    workload_ports = workload_runner.RDBPorts(host=database_server.host, http_port=database_server.http_port, rdb_port=database_server.driver_port, db_name=dbName, table_name=tableName)
    with workload_runner.ContinuousWorkload(opts["workload"], workload_ports) as workload:
        workload.start()

        print("Running workload for 10 seconds (%.2fs)" % (time.time() - startTime))
        time.sleep(10)
        cluster.check()
        assert [] == list(r.db('rethinkdb').table('issues').run(conn))

        print("Killing the access server (%.2fs)" % (time.time() - startTime))
        access_server.kill()

        issues = list(r.db('rethinkdb').table('issues').run(conn))
        time.sleep(.5)
        assert len(issues) > 0, 'Issue was not raised when the server stopped'
        assert len(issues) == 1, 'There were extra cluster issues when server stopped: %s' % repr(issues)

        # Don't bother stopping the workload, just exit and it will get killed

    sys.stderr.write('before kill: %s\n' % repr(list(r.db(dbName).table_config(tableName).run(conn))))
    print("Removing the stopped server (%.2fs)" % (time.time() - startTime))

    r.db('rethinkdb').table('server_config').get(access_server.uuid).delete().run(conn)
    time.sleep(.5)

    sys.stderr.write('after kill: %s\n' % repr(list(r.db(dbName).table_config(tableName).run(conn))))

    issues = list(r.db('rethinkdb').table('issues').run(conn))
    assert [] == issues, 'The issues list was not empty: %s' % repr(issues)
    database_server.check()

    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))
