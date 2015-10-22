#!/usr/bin/env python
# Copyright 2012-2015 RethinkDB, all rights reserved.

import sys, os, pprint

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()

cycles = 10

# == start first instance of server

utils.print_with_time("Starting server")
with driver.Process(console_output=True, name='.', command_prefix=command_prefix, extra_options=serve_options, wait_until_ready=True) as server:
    
    utils.print_with_time("Establishing ReQL connection")
    conn = r.connect(host=server.host, port=server.driver_port)
    
    utils.print_with_time("Creating db/table %s/%s" % (dbName, tableName))
    if dbName not in r.db_list().run(conn):
        r.db_create(dbName).run(conn)
    if tableName in r.db(dbName).table_list().run(conn):
        r.db(dbName).table_drop(tableName).run(conn)
    r.db(dbName).table_create(tableName).run(conn)
    
    utils.print_with_time("Collecting metadata for first run")
    initalMetadata = r.db('rethinkdb').table('server_config').get(server.uuid).run(conn)
    
    for i in range(1, cycles + 1):
        utils.print_with_time('Starting recycle %d' % i)
        server.stop()
        server.start()
        conn = r.connect(host=server.host, port=server.driver_port)
        metaData = r.db('rethinkdb').table('server_config').get(server.uuid).run(conn)
        assert metaData == initalMetadata, "The server metadata did not match between runs:\n%s\nvs. inital:\n%s" % (pprint.pformat(beforeMetaData), pprint.pformat(afterMetaData))
utils.print_with_time("Done.")
