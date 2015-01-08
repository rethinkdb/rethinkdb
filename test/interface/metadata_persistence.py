#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

from __future__ import print_function

import sys, os, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()

beforeMetaData = None
afterMetaData = None
files = None

# == start first instance of server

print("Starting server (%.2fs)" % (time.time() - startTime))
with driver.Process(console_output=True, output_folder='.', command_prefix=command_prefix, extra_options=serve_options, wait_until_ready=True) as server:
    
    files = server.files
    
    print("Establishing ReQL connection (%.2fs)" % (time.time() - startTime))
    
    conn = r.connect(host=server.host, port=server.driver_port)
    
    print("Creating db/table %s/%s (%.2fs)" % (dbName, tableName, time.time() - startTime))
    
    if dbName not in r.db_list().run(conn):
        r.db_create(dbName).run(conn)
    
    if tableName in r.db(dbName).table_list().run(conn):
        r.db(dbName).table_drop(tableName).run(conn)
    r.db(dbName).table_create(tableName).run(conn)
    
    print("Collecting metadata for first run (%.2fs)" % (time.time() - startTime))
    
    beforeMetaData = r.db('rethinkdb').table('server_config').get(server.uuid).run(conn)
    
    print("Shutting down server (%.2fs)" % (time.time() - startTime))

print("Restarting server with same files (%.2fs)" % (time.time() - startTime))
with driver.Process(files=files, console_output=True, command_prefix=command_prefix, extra_options=serve_options, wait_until_ready=True) as server:
    
    print("Establishing second ReQL connection (%.2fs)" % (time.time() - startTime))
    
    conn = r.connect(host=server.host, port=server.driver_port)
    
    print("Collecting metadata for second run (%.2fs)" % (time.time() - startTime))
    
    afterMetaData = r.db('rethinkdb').table('server_config').get(server.uuid).run(conn)
    
    assert afterMetaData == beforeMetaData, "The server metadata did not match between runs:\n%s\nvs.\n%s" % (str(beforeMetaData), str(afterMetaData))

    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))
