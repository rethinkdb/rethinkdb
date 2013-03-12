###
# Tests the driver API for making connections and excercizes the networking code
###

from sys import argv
from subprocess import Popen
from time import sleep
from sys import path
import unittest
path.append('.')
from test_util import RethinkDBTestServers
path.append("../../drivers/python")

# We import the module both ways because this used to crash and we
# need to test for it
from rethinkdb import *
import rethinkdb as r

server_build = argv[1]
use_default_port = bool(int(argv[2]))

class TestNoConnection(unittest.TestCase):
    # No servers started yet so this should fail
    def test_connect(self):
        self.assertRaisesRegexp(
            RqlDriverError, "Could not connect to localhost:28015.",
            r.connect)

    def test_connect_port(self):
        self.assertRaisesRegexp(
            RqlDriverError, "Could not connect to localhost:11221.",
            r.connect, port=11221)

    def test_connect_host(self):
        self.assertRaisesRegexp(
            RqlDriverError, "Could not connect to 0.0.0.0:28015.",
            r.connect, host="0.0.0.0")

    def test_connect_host(self):
        self.assertRaisesRegexp(
            RqlDriverError, "Could not connect to 0.0.0.0:11221.",
            r.connect, host="0.0.0.0", port=11221)

class TestConnectionDefaultPort(unittest.TestCase):

    def setUp(self):
        if not use_default_port:
            skipTest("Not testing default port")
        self.servers = RethinkDBTestServers(server_build=server_build, use_default_port=use_default_port)
        self.servers.__enter__()

    def tearDown(self):
        self.default_server.__exit__(None, None, None)

    def test_connect(self):
        conn = r.connect()
        conn.reconnect()

    def test_connect_host(self):
        conn = r.connect(host='localhost')
        conn.reconnect()

    def test_connect_host_port(self):
        conn = r.connect(host='localhost', port=28015)
        conn.reconnect()

    def test_connect_port(self):
        conn = r.connect(port=28015)
        conn.reconnect()

class TestWithConnection(unittest.TestCase):

    def setUp(self):
        self.servers = RethinkDBTestServers(server_build=server_build)
        self.servers.__enter__()
        self.port = self.servers.cpp_port

    def tearDown(self):
        self.servers.__exit__(None, None, None)

class TestConnection(TestWithConnection):
    def test_connect_close_reconnect(self):
        c = r.connect(port=self.port)
        r.expr(1).run(c)
        c.close()
        c.close()
        c.reconnect()
        r.expr(1).run(c)

    def test_connect_close_expr(self):
        c = r.connect(port=self.port)
        r.expr(1).run(c)
        c.close()
        self.assertRaisesRegexp(
            r.RqlDriverError, "Connection is closed.",
            r.expr(1).run, c)

    def test_db(self):
        c = r.connect(port=self.port)

        r.db('test').table_create('t1').run(c)
        r.db_create('db2').run(c)
        r.db('db2').table_create('t2').run(c)

        # Default db should be 'test' so this will work
        r.table('t1').run(c)

        # Use a new database
        c.use('db2')
        r.table('t2').run(c)

        c.use('test')
        self.assertRaisesRegexp(
            r.RqlRuntimeError, "Table `t2` does not exist.",
            r.table('t2').run, c)

class TestShutdown(TestWithConnection):
    def test_shutdown(self):
        c = r.connect(port=self.port)
        r.expr(1).run(c)
        self.servers.stop()
        sleep(0.2)
        self.assertRaisesRegexp(
            r.RqlDriverError, "Connection is closed.",
            r.expr(1).run, c)

# # TODO: test cursors, streaming large values

if __name__ == '__main__':
    print "Running py connection tests"
    suite = unittest.TestSuite()
    loader = unittest.TestLoader()
    suite.addTest(loader.loadTestsFromTestCase(TestNoConnection))
    suite.addTest(loader.loadTestsFromTestCase(TestConnection))
    suite.addTest(loader.loadTestsFromTestCase(TestShutdown))
    unittest.TextTestRunner(verbosity=1).run(suite)
