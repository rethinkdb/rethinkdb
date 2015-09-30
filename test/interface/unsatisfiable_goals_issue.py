#!/usr/bin/env python
# Copyright 2010-2015 RethinkDB, all rights reserved.

import os, sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

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

utils.print_with_time("Starting a server")
with driver.Process(name='.', command_prefix=command_prefix, extra_options=serve_options, wait_until_ready=True) as server:
    
    utils.print_with_time("Establishing ReQL Connection")
    
    conn = r.connect(host=server.host, port=server.driver_port)
    
    utils.print_with_time("Creating db/table %s/%s" % (dbName, tableName))
    
    if dbName not in r.db_list().run(conn):
        r.db_create(dbName).run(conn)
    
    if tableName in r.db(dbName).table_list().run(conn):
        r.db(dbName).table_drop(tableName).run(conn)
    r.db(dbName).table_create(tableName).run(conn)
    
    utils.print_with_time("Trying to set impossible goals with reconfigure")
    
    assertRaises(r.ReqlRuntimeError, r.db(dbName).table(tableName).reconfigure(shards=1, replicas=4).run, conn)
    
    utils.print_with_time("Trying to set impossible goals through table.config")

    assert r.db(dbName).table(tableName).config().update({'shards':[{'primary_replica':server.name, 'replicas':[server.name, server.name, server.name, server.name]}]})['errors'].run(conn) == 1
    assert r.db(dbName).table(tableName).config().update({'write_acks':[{'acks':'majority', 'replicas':['larkost_local_kxj', 'alpha']}]})['errors'].run(conn) == 1
    
    utils.print_with_time("Trying to set impossible goals through rethinkdb.table_config")
    
    assert r.db('rethinkdb').table('table_config').filter({'name':tableName}).nth(0).update({'shards':[{'primary_replica':server.name, 'replicas':[server.name, server.name, server.name, server.name]}]})['errors'].run(conn) == 1
    
    utils.print_with_time("Checking server up")
    
    server.check()
    issues = list(r.db('rethinkdb').table('current_issues').run(conn))
    assert [] == issues, 'The issues list was not empty: %s' % repr(issues)
    
    utils.print_with_time("Cleaning up")
utils.print_with_time("Done.")
