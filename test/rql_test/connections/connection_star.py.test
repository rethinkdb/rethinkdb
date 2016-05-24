#!/usr/bin/env python

'''Basic test that `from rethinkdb import *` works'''

import os, sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.realpath(__file__)), os.pardir, os.pardir, "common"))
import driver, utils

dbName, tableName = utils.get_test_db_table()

# -- import rethikndb driver via star method

proto_r = utils.import_python_driver()
sys.path.insert(0, os.path.dirname(os.path.realpath(proto_r.__file__)))
from rethinkdb import *

# -- import tests

assert r == rethinkdb
assert issubclass(r, object)
assert issubclass(rethinkdb, object)

"""ReqlCursorEmpty
ReqlError
    ReqlCompileError
    ReqlRuntimeError
    ReqlQueryLogicError
        ReqlNonExistenceError
    ReqlResourceLimitError
    ReqlUserError
    ReqlInternalError
    ReqlTimeoutError
    ReqlAvailabilityError
        ReqlOpFailedError
        ReqlOpIndeterminateError
    ReqlDriverError
        ReqlAuthError"""

assert issubclass(ReqlCursorEmpty, Exception)
assert issubclass(ReqlError, Exception)
assert issubclass(ReqlCompileError, ReqlError)
assert issubclass(ReqlRuntimeError, ReqlError)
assert issubclass(ReqlRuntimeError, ReqlError)
assert issubclass(ReqlNonExistenceError, ReqlQueryLogicError)
assert issubclass(ReqlResourceLimitError, ReqlError)
assert issubclass(ReqlUserError, ReqlError)
assert issubclass(ReqlInternalError, ReqlError)
assert issubclass(ReqlTimeoutError, ReqlError)
assert issubclass(ReqlAvailabilityError, ReqlError)
assert issubclass(ReqlOpFailedError, ReqlAvailabilityError)
assert issubclass(ReqlOpIndeterminateError, ReqlAvailabilityError)
assert issubclass(ReqlDriverError, ReqlError)
assert issubclass(ReqlAuthError, ReqlDriverError)

assert issubclass(RqlError, Exception)
assert issubclass(RqlClientError, RqlError)
assert issubclass(RqlCompileError, RqlError)
assert issubclass(RqlRuntimeError, RqlError)
assert issubclass(RqlDriverError, Exception)

# -- simple tests

with driver.Process(wait_until_ready=True) as server:
    
    # - connect
    
    r.connect(host=server.host, port=server.driver_port)
    conn = rethinkdb.connect(host=server.host, port=server.driver_port)
    
    # - create database
    
    if dbName not in r.db_list().run(conn):
        r.db_create(dbName).run(conn)
    
    # - create table
    
    if tableName in r.db(dbName).table_list().run(conn):
        r.db(dbName).table_drop(tableName).run(conn)
    r.db(dbName).table_create(tableName).run(conn)
    
    # - simple querys
    
    r.db_list().run(conn)
    rethinkdb.db_list().run(conn)
    
    assert len(r.db(dbName).table_list().run(conn)) > 0
    assert len(rethinkdb.db(dbName).table_list().run(conn)) > 0
