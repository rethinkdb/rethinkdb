#!/usr/bin/env python

import datetime
import os
import random
import re
import sys
import tempfile
import time
import traceback
import unittest
import functools
import socket

sys.path.insert(0, os.path.join(os.path.dirname(os.path.realpath(__file__)), os.pardir, "common"))

import driver
import utils

# -- import the rethinkdb driver

r = utils.import_python_driver()

if 'RDB_DRIVER_PORT' in os.environ:
    sharedServerDriverPort = int(os.environ['RDB_DRIVER_PORT'])
    if 'RDB_SERVER_HOST' in os.environ:
        sharedServerHost = os.environ['RDB_SERVER_HOST']
    else:
        sharedServerHost = 'localhost'
else:
    raise Exception("RDB_DRIVER_PORT must be set.")
    
# -- test classes

class TestPermissionsBase(unittest.TestCase):
    adminConn = None
    userConn = None
    db = None
    tbl = None

    def assertNoPermissions(self, query):
        try:
            query.run(self.userConn)
            raise Exception("Expected permission error when running %s", query)
        except r.ReqlPermissionError:
            pass

    def assertPermissions(self, query):
        query.run(self.userConn)

    def setPermissions(self, scope, permissions):
        # Reset the permissions on this scope first:
        scope.grant("user", {"read": None, "write": None, "config": None}).run(self.adminConn)
        try:
            scope.grant("user", {"connect": None}).run(self.adminConn)
        except r.ReqlOpFailedError:
            pass

        # Grant the new permissions:
        scope.grant("user", permissions).run(self.adminConn)

        # Wait for the change to be visible on the `user` connection:
        # TODO: Is there a better way to do this?
        time.sleep(0.1)

    def setUp(self):
        self.adminConn = r.connect(host=sharedServerHost, port=sharedServerDriverPort, user="admin", password="")

        # Create a regular user account that we will be using during testing
        res = r.db('rethinkdb').table('users').insert({"id": "user", "password": "secret"}).run(self.adminConn)
        if res != {'skipped': 0, 'deleted': 0, 'unchanged': 0, 'errors': 0, 'replaced': 0, 'inserted': 1}:
            raise Exception('Unable to create user `user`, got: %s' % str(res))

        # Also create a test database and table
        res = r.db_create('perm').pluck("dbs_created").run(self.adminConn)
        if res != {'dbs_created': 1}:
            raise Exception('Unable to create database `perm`, got: %s' % str(res))
        self.db = r.db('perm')
        res = self.db.table_create("test").pluck("tables_created").run(self.adminConn)
        if res != {'tables_created': 1}:
            raise Exception('Unable to create table `test`, got: %s' % str(res))
        self.tbl = self.db.table('test')

        self.userConn = r.connect(host=sharedServerHost, port=sharedServerDriverPort, user="user", password="secret")

    def tearDown(self):
        if self.adminConn is not None:
            r.db('rethinkdb').table('permissions').between(["user"], ["user", r.maxval, r.maxval]).delete().run(self.adminConn)
            r.db('rethinkdb').table('users').get("user").delete().run(self.adminConn)
            r.db_drop('perm').run(self.adminConn)
            self.adminConn.close()
        if self.userConn is not None:
            self.userConn.close()

class TestRealTablePermissions(TestPermissionsBase):
    def test_read_scopes(self):
        self.assertNoPermissions(self.tbl)

        self.setPermissions(self.tbl, {"read": True})
        self.assertPermissions(self.tbl)
        self.setPermissions(self.tbl, {"read": None})
        self.assertNoPermissions(self.tbl)

        self.setPermissions(self.db, {"read": True})
        self.assertPermissions(self.tbl)
        self.setPermissions(self.tbl, {"read": False})
        self.assertNoPermissions(self.tbl)
        self.setPermissions(self.tbl, {"read": None})
        self.setPermissions(self.db, {"read": None})
        self.assertNoPermissions(self.tbl)

        self.setPermissions(r, {"read": True})
        self.assertPermissions(self.tbl)
        self.setPermissions(self.tbl, {"read": False})
        self.assertNoPermissions(self.tbl)
        self.setPermissions(self.tbl, {"read": None})
        self.setPermissions(self.db, {"read": False})
        self.assertNoPermissions(self.tbl)
        self.setPermissions(self.db, {"read": None})
        self.setPermissions(r, {"read": None})
        self.assertNoPermissions(self.tbl)

    def test_read_special(self):
        # These work even without permissions:
        self.assertPermissions(self.tbl.status())
        self.assertPermissions(self.tbl.index_list())
        self.assertPermissions(self.tbl.index_wait())
        self.assertPermissions(self.tbl.wait())
        # These should only work with permissions:
        self.assertNoPermissions(self.tbl.info())
        self.assertNoPermissions(self.tbl.is_empty())
        self.assertNoPermissions(self.tbl.count())

        self.setPermissions(self.tbl, {"read": True})
        self.assertPermissions(self.tbl.info())
        self.assertPermissions(self.tbl.status())
        self.assertPermissions(self.tbl.index_list())
        self.assertPermissions(self.tbl.index_wait())
        self.assertPermissions(self.tbl.wait())
        self.assertPermissions(self.tbl.is_empty())
        self.assertPermissions(self.tbl.count())
        self.assertPermissions(self.tbl)
        self.setPermissions(self.tbl, {"read": None})

    def test_write(self):
        self.assertNoPermissions(self.tbl.insert({"id": "a"}))
        self.assertNoPermissions(self.tbl.update({"f": "v"}))
        self.assertNoPermissions(self.tbl.replace({"id": "a", "f": "v"}))
        self.assertNoPermissions(self.tbl.delete())

        self.setPermissions(self.tbl, {"read": True})
        self.assertNoPermissions(self.tbl.insert({"id": "a"}))
        # These are ok if the table is empty
        self.assertPermissions(self.tbl.update({"f": "v"}))
        self.assertPermissions(self.tbl.replace({"id": "a", "f": "v"}))
        self.assertPermissions(self.tbl.delete())
        # ... but not if they actually have an effect:
        res = self.tbl.insert({"id": "a"}).pluck("inserted").run(self.adminConn)
        if res != {'inserted': 1}:
            raise Exception('Failed to insert data, got: %s' % str(res))
        self.assertNoPermissions(self.tbl.update({"f": "v"}))
        self.assertNoPermissions(self.tbl.replace({"id": "a", "f": "v"}))
        self.assertNoPermissions(self.tbl.delete())
        self.setPermissions(self.tbl, {"read": None})

        self.setPermissions(self.tbl, {"read": True, "write": True})
        self.assertPermissions(self.tbl.insert({"id": "a"}))
        self.assertPermissions(self.tbl.update({"f": "v"}))
        self.assertPermissions(self.tbl.replace({"id": "a", "f": "v"}))
        self.assertPermissions(self.tbl.delete())
        self.setPermissions(self.tbl, {"read": None, "write": None})

        # Even pure writes also require read permissions (otherwise
        # something like `return_changes` could be used to leak data).
        self.setPermissions(self.tbl, {"write": True})
        self.assertNoPermissions(self.tbl.insert({"id": "a"}))
        self.setPermissions(self.tbl, {"write": None})

    def test_write_special(self):
        self.assertNoPermissions(self.tbl.sync())

        self.setPermissions(self.tbl, {"write": True, "read": True})
        self.assertPermissions(self.tbl.sync())
        self.setPermissions(self.tbl, {"write": None, "read": None})

    def test_config(self):
        self.assertNoPermissions(self.tbl.config())
        self.assertNoPermissions(self.tbl.reconfigure(shards=2, replicas=1))
        self.assertNoPermissions(self.tbl.rebalance())
        self.assertNoPermissions(self.tbl.index_create("a"))
        self.assertNoPermissions(self.tbl.index_drop("a"))
        self.assertNoPermissions(self.db.table_create("t"))
        self.assertNoPermissions(r.db_create("d"))

        self.setPermissions(self.tbl, {"config": True})
        self.assertPermissions(self.tbl.config())
        self.assertPermissions(self.tbl.reconfigure(shards=2, replicas=1))
        self.assertPermissions(self.tbl.rebalance())
        self.assertPermissions(self.tbl.index_create("a"))
        self.assertPermissions(self.tbl.index_drop("a"))
        self.assertNoPermissions(self.db.table_create("t"))
        res = self.db.table_create("t").pluck("tables_created").run(self.adminConn)
        if res != {'tables_created': 1}:
            raise Exception('Unable to create table `t`, got: %s' % str(res))
        self.assertNoPermissions(self.db.table_drop("t"))
        res = self.db.table_drop("t").pluck("tables_dropped").run(self.adminConn)
        if res != {'tables_dropped': 1}:
            raise Exception('Unable to drop table `t`, got: %s' % str(res))
        self.assertNoPermissions(r.db_create("d"))
        res = r.db_create("d").pluck("dbs_created").run(self.adminConn)
        if res != {'dbs_created': 1}:
            raise Exception('Unable to create DB `d`, got: %s' % str(res))
        self.assertNoPermissions(r.db_drop("d"))
        res = r.db_drop("d").pluck("dbs_dropped").run(self.adminConn)
        if res != {'dbs_dropped': 1}:
            raise Exception('Unable to drop DB `d`, got: %s' % str(res))
        self.setPermissions(self.tbl, {"config": None})

        self.setPermissions(self.db, {"config": True})
        self.assertPermissions(self.db.table_create("t"))
        self.assertPermissions(self.db.table_drop("t"))
        self.assertNoPermissions(r.db_create("d"))
        res = r.db_create("d").pluck("dbs_created").run(self.adminConn)
        if res != {'dbs_created': 1}:
            raise Exception('Unable to create DB `d`, got: %s' % str(res))
        self.assertNoPermissions(r.db_drop("d"))
        res = r.db_drop("d").pluck("dbs_dropped").run(self.adminConn)
        if res != {'dbs_dropped': 1}:
            raise Exception('Unable to drop DB `d`, got: %s' % str(res))
        self.setPermissions(self.db, {"config": None})

        self.setPermissions(r, {"config": True})
        self.assertPermissions(self.db.table_create("t"))
        self.assertPermissions(self.db.table_drop("t"))
        self.assertPermissions(r.db_create("d"))
        self.assertPermissions(r.db_drop("d"))
        self.setPermissions(r, {"config": None})

    def test_connect(self):
        self.assertNoPermissions(r.http("http://localhost:12345").default(None))

        self.setPermissions(r, {"connect": True})
        self.assertPermissions(r.http("http://localhost:12345").default(None))
        self.setPermissions(r, {"connect": None})

class TestSpecialCases(TestPermissionsBase):
    def test_db_change(self):
        # Special case: A user must not be able to move a table into a
        # database if they don't have config permissions on both the old
        # and the new database.
        self.setPermissions(self.db, {"config": True})
        res = r.db_create("d").pluck("dbs_created").run(self.adminConn)
        if res != {'dbs_created': 1}:
            raise Exception('Unable to create DB `d`, got: %s' % str(res))
        res = self.tbl.config().update({"db": "d"}).pluck("first_error").run(self.userConn)
        if res != {'first_error': "Only administrators may move a table to a different database."}:
            raise Exception('Changing the database did not fail, got: %s' % str(res))
        res = r.db_drop("d").pluck("dbs_dropped").run(self.adminConn)
        if res != {'dbs_dropped': 1}:
            raise Exception('Unable to drop DB `d`, got: %s' % str(res))
        self.setPermissions(self.db, {"config": None})

class TestSystemTablePermissions(TestPermissionsBase):
    def test_system(self):
        self.setPermissions(self.db, {"config": True, "write": True, "read": True})
        self.assertNoPermissions(r.db('rethinkdb').table('users'))
        self.assertNoPermissions(r.db('rethinkdb').table('permissions'))
        self.assertNoPermissions(r.db('rethinkdb').table('cluster_config'))
        self.assertNoPermissions(r.db('rethinkdb').table('current_issues'))
        self.assertNoPermissions(r.db('rethinkdb').table('db_config'))
        self.assertNoPermissions(r.db('rethinkdb').table('jobs'))
        self.assertNoPermissions(r.db('rethinkdb').table('logs'))
        self.assertNoPermissions(r.db('rethinkdb').table('server_config'))
        self.assertNoPermissions(r.db('rethinkdb').table('server_status'))
        self.assertNoPermissions(r.db('rethinkdb').table('stats'))
        self.assertNoPermissions(r.db('rethinkdb').table('table_config'))
        self.assertNoPermissions(r.db('rethinkdb').table('table_status'))

        self.assertNoPermissions(r.db('rethinkdb').table('users').insert({}))
        self.assertNoPermissions(r.db('rethinkdb').table('permissions').insert({}))
        self.assertNoPermissions(r.db('rethinkdb').table('cluster_config').insert({}))
        self.assertNoPermissions(r.db('rethinkdb').table('current_issues').insert({}))
        self.assertNoPermissions(r.db('rethinkdb').table('db_config').insert({}))
        self.assertNoPermissions(r.db('rethinkdb').table('jobs').insert({}))
        self.assertNoPermissions(r.db('rethinkdb').table('logs').insert({}))
        self.assertNoPermissions(r.db('rethinkdb').table('server_config').insert({}))
        self.assertNoPermissions(r.db('rethinkdb').table('server_status').insert({}))
        self.assertNoPermissions(r.db('rethinkdb').table('stats').insert({}))
        self.assertNoPermissions(r.db('rethinkdb').table('table_config').insert({}))
        self.assertNoPermissions(r.db('rethinkdb').table('table_status').insert({}))
        self.setPermissions(self.db, {"config": None, "write": None, "read": None})

# -- Main function

if __name__ == '__main__':
    print("Running basic permissions test")
    suite = unittest.TestSuite()
    loader = unittest.TestLoader()
    suite.addTest(loader.loadTestsFromTestCase(TestRealTablePermissions))
    suite.addTest(loader.loadTestsFromTestCase(TestSpecialCases))
    suite.addTest(loader.loadTestsFromTestCase(TestSystemTablePermissions))

    res = unittest.TextTestRunner(stream=sys.stdout, verbosity=2).run(suite)
    if not res.wasSuccessful():
        sys.exit(1)
