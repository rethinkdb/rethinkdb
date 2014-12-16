#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

from __future__ import print_function

import os, sys, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()    

def assertRaises(exception, funct, *args, **kwds):
    assert issubclass(exception, Exception)
    result = None
    try:
        result = funct(*args, **kwds)
    except exception:
        pass
    else:
        if result is None:
            raise Exception('Should have raised %s, but did not' % exception.__class__.__name__)
        else:
            raise Exception('Should have raised %s, but rather returned: %s' % (exception.__class__.__name__, repr(result)))

print("Starting a server (%.2fs)" % (time.time() - startTime))
with driver.Process(output_folder='.', command_prefix=command_prefix, extra_options=serve_options, wait_until_ready=True) as server:
    
    print("Establishing ReQL Connection (%.2fs)" % (time.time() - startTime))
    
    conn = r.connect(host=server.host, port=server.driver_port)
    
    print("Creating db/table %s/%s (%.2fs)" % (dbName, tableName, time.time() - startTime))
    
    if dbName not in r.db_list().run(conn):
        r.db_create(dbName).run(conn)
    
    if tableName in r.db(dbName).table_list().run(conn):
        r.db(dbName).table_drop(tableName).run(conn)
    r.db(dbName).table_create(tableName).run(conn)
    
    print("Trying to set impossible goals with reconfigure (%.2fs)" % (time.time() - startTime))
    
    assertRaises(r.RqlRuntimeError, r.db(dbName).table(tableName).reconfigure(shards=1, replicas=4).run, conn)
    
    print("Trying to set impossible goals through table_config (%.2fs)" % (time.time() - startTime))
    
    assert r.db(dbName).table_config(tableName).update({'shards':[{'primary_replica':server.name, 'replicas':[server.name, server.name, server.name, server.name]}]})['errors'].run(conn) == 1
    assert r.db(dbName).table_config(tableName).update({'write_acks':[{'acks':'majority', 'replicas':['larkost_local_kxj', 'alpha']}]})['errors'].run(conn) == 1
    
    print("Trying to set impossible goals through rethinkdb.table_config (%.2fs)" % (time.time() - startTime))
    
    assert r.db('rethinkdb').table('table_config').filter({'name':tableName}).nth(0).update({'shards':[{'primary_replica':server.name, 'replicas':[server.name, server.name, server.name, server.name]}]})['errors'].run(conn) == 1
    
    print("Checking server up (%.2fs)" % (time.time() - startTime))
    
    server.check()
    issues = list(r.db('rethinkdb').table('issues').run(conn))
    assert [] == issues, 'The issues list was not empty: %s' % repr(issues)
    
    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))
