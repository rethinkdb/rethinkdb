##
# Tests the driver API for making connections and excercizes the networking code
###

import socket
import threading
import SocketServer
import datetime
import sys
from time import sleep, time
import os
import unittest
sys.path.insert(0, os.path.join(os.path.dirname(os.path.realpath(__file__)), os.pardir))
import test_util

sys.path.insert(0, os.path.join(os.path.dirname(os.path.realpath(__file__)), os.pardir, os.pardir, "common"))
import utils
sys.path.insert(0, os.path.join(utils.project_root_dir(), 'drivers', 'python'))

# We import the module both ways because this used to crash and we need to test for it
from rethinkdb import *
import rethinkdb as r

server_build_dir = sys.argv[1]
use_default_port = 0
if len(sys.argv) > 2:
    use_default_port = bool(int(sys.argv[2]))

class TestNoConnection(unittest.TestCase):
    # No servers started yet so this should fail
    def test_connect(self):
        if not use_default_port:
            self.skipTest("Not testing default port")
        self.assertRaisesRegexp(
            RqlDriverError, "Could not connect to localhost:28015.",
            r.connect)

    def test_connect_port(self):
        self.assertRaisesRegexp(
            RqlDriverError, "Could not connect to localhost:11221.",
            r.connect, port=11221)

    def test_connect_host(self):
        if not use_default_port:
            self.skipTest("Not testing default port")
        self.assertRaisesRegexp(
            RqlDriverError, "Could not connect to 0.0.0.0:28015.",
            r.connect, host="0.0.0.0")

    def test_connect_host(self):
        self.assertRaisesRegexp(
            RqlDriverError, "Could not connect to 0.0.0.0:11221.",
            r.connect, host="0.0.0.0", port=11221)

    def test_empty_run(self):
        # Test the error message when we pass nothing to run and didn't call `repl`
        self.assertRaisesRegexp(
            r.RqlDriverError, "RqlQuery.run must be given a connection to run on.",
            r.expr(1).run)

    def test_auth_key(self):
        # Test that everything still doesn't work even with an auth key
        if not use_default_port:
            self.skipTest("Not testing default port")
        self.assertRaisesRegexp(
            RqlDriverError, 'Could not connect to 0.0.0.0:28015."',
            r.connect, host="0.0.0.0", port=28015, auth_key="hunter2")

class TestConnectionDefaultPort(unittest.TestCase):
    
    servers = None
    
    def setUp(self):
        if not use_default_port:
            self.skipTest("Not testing default port")
        self.servers = test_util.RethinkDBTestServers(4, server_build_dir=server_build_dir, use_default_port=use_default_port)
        self.servers.__enter__()

    def tearDown(self):
        if self.servers is not None:
            self.servers.__exit__(None, None, None)

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

    def test_connect_wrong_auth(self):
        self.assertRaisesRegexp(
            RqlDriverError, "Server dropped connection with message: \"ERROR: Incorrect authorization key.\"",
            r.connect, auth_key="hunter2")

class BlackHoleRequestHandler(SocketServer.BaseRequestHandler):
    def handle(self):
        sleep(1)

class ThreadedBlackHoleServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
    pass

class TestTimeout(unittest.TestCase):
    def setUp(self):
        import random # importing here to avoid issue #2343
        self.timeout = 0.5
        self.port = random.randint(1025, 65535)

        self.server = ThreadedBlackHoleServer(('localhost', self.port), BlackHoleRequestHandler)
        self.server_thread = threading.Thread(target=self.server.serve_forever)
        self.server_thread.start()

    def tearDown(self):
        self.server.shutdown()

    def test_timeout(self):
        self.assertRaises(socket.timeout, r.connect, port=self.port, timeout=self.timeout)

class TestAuthConnection(unittest.TestCase):

    def setUp(self):
        self.servers = test_util.RethinkDBTestServers(4, server_build_dir=server_build_dir)
        self.servers.__enter__()
        self.port=self.servers.driver_port()

        cluster_port = self.servers.cluster_port()
        exe = self.servers.executable()

        if test_util.set_auth(cluster_port, exe, "hunter2") != 0:
            raise RuntimeError("Could not set up authorization key")

    def tearDown(self):
        if self.servers is not None:
            self.servers.__exit__()

    def test_connect_no_auth(self):
        self.assertRaisesRegexp(
            RqlDriverError, "Server dropped connection with message: \"ERROR: Incorrect authorization key.\"",
            r.connect, port=self.port)

    def test_connect_wrong_auth(self):
        self.assertRaisesRegexp(
            RqlDriverError, "Server dropped connection with message: \"ERROR: Incorrect authorization key.\"",
            r.connect, port=self.port, auth_key="")

        self.assertRaisesRegexp(
            RqlDriverError, "Server dropped connection with message: \"ERROR: Incorrect authorization key.\"",
            r.connect, port=self.port, auth_key="hunter3")

        self.assertRaisesRegexp(
            RqlDriverError, "Server dropped connection with message: \"ERROR: Incorrect authorization key.\"",
            r.connect, port=self.port, auth_key="hunter22")

    def test_connect_long_auth(self):
        long_key = str("k") * 2049
        not_long_key = str("k") * 2048

        self.assertRaisesRegexp(
            RqlDriverError, "Server dropped connection with message: \"ERROR: Client provided an authorization key that is too long.\"",
            r.connect, port=self.port, auth_key=long_key)

        self.assertRaisesRegexp(
            RqlDriverError, "Server dropped connection with message: \"ERROR: Incorrect authorization key.\"",
            r.connect, port=self.port, auth_key=not_long_key)

    def test_connect_correct_auth(self):
        conn = r.connect(port=self.port, auth_key="hunter2")
        conn.reconnect()

class TestWithConnection(unittest.TestCase):

    def setUp(self):
        self.servers = test_util.RethinkDBTestServers(4, server_build_dir=server_build_dir)
        self.servers.__enter__()
        self.port = self.servers.driver_port()
        conn = r.connect(port=self.port)
        if 'test' not in r.db_list().run(conn):
            r.db_create('test').run(conn)

    def tearDown(self):
        if self.servers is not None:
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

    def test_noreply_wait_waits(self):
        c = r.connect(port=self.port)
        t = time()
        r.js('while(true);', timeout=0.5).run(c, noreply=True)
        c.noreply_wait()
        duration = time() - t
        self.assertGreaterEqual(duration, 0.5)

    def test_close_waits_by_default(self):
        c = r.connect(port=self.port)
        t = time()
        r.js('while(true);', timeout=0.5).run(c, noreply=True)
        c.close()
        duration = time() - t
        self.assertGreaterEqual(duration, 0.5)

    def test_reconnect_waits_by_default(self):
        c = r.connect(port=self.port)
        t = time()
        r.js('while(true);', timeout=0.5).run(c, noreply=True)
        c.reconnect()
        duration = time() - t
        self.assertGreaterEqual(duration, 0.5)

    def test_close_does_not_wait_if_requested(self):
        c = r.connect(port=self.port)
        t = time()
        r.js('while(true);', timeout=0.5).run(c, noreply=True)
        c.close(noreply_wait=False)
        duration = time() - t
        self.assertLess(duration, 0.5)

    def test_reconnect_does_not_wait_if_requested(self):
        c = r.connect(port=self.port)
        t = time()
        r.js('while(true);', timeout=0.5).run(c, noreply=True)
        c.reconnect(noreply_wait=False)
        duration = time() - t
        self.assertLess(duration, 0.5)

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
        self.assertRaisesRegexp(
            r.RqlRuntimeError, "Table `db2.t1` does not exist.",
            r.table('t1').run, c)

        c.use('test')
        r.table('t1').run(c)
        self.assertRaisesRegexp(
            r.RqlRuntimeError, "Table `test.t2` does not exist.",
            r.table('t2').run, c)

        c.close()

        # Test setting the db in connect
        c = r.connect(db='db2', port=self.port)
        r.table('t2').run(c)

        self.assertRaisesRegexp(
            r.RqlRuntimeError, "Table `db2.t1` does not exist.",
            r.table('t1').run, c)

        c.close()

        # Test setting the db as a `run` option
        c = r.connect(port=self.port)
        r.table('t2').run(c, db='db2')

    def test_use_outdated(self):
        c = r.connect(port=self.port)
        r.db('test').table_create('t1').run(c)

        # Use outdated is an option that can be passed to db.table or `run`
        # We're just testing here if the server actually accepts the option.

        r.table('t1', use_outdated=True).run(c)
        r.table('t1').run(c, use_outdated=True)

    def test_repl(self):

        # Calling .repl() should set this connection as global state
        # to be used when `run` is not otherwise passed a connection.
        c = r.connect(port=self.port).repl()

        r.expr(1).run()

        c.repl() # is idempotent

        r.expr(1).run()

        c.close()

        self.assertRaisesRegexp(
            r.RqlDriverError, "Connection is closed",
            r.expr(1).run)

    def test_port_conversion(self):
        c = r.connect(port=str(self.port))
        r.expr(1).run(c)

        c.close()
        self.assertRaisesRegexp(
            r.RqlDriverError,
            "Could not convert port abc to an integer.",
            lambda: r.connect(port='abc'))

class TestShutdown(TestWithConnection):
    def test_shutdown(self):
        c = r.connect(port=self.port)
        r.expr(1).run(c)
        self.servers.stop()
        sleep(0.2)
        self.assertRaisesRegexp(
            r.RqlDriverError, "Connection is closed.",
            r.expr(1).run, c)


# This doesn't really have anything to do with connections but it'll go
# in here for the time being.
class TestPrinting(unittest.TestCase):

    # Just test that RQL queries support __str__ using the pretty printer.
    # An exhaustive test of the pretty printer would be, well, exhausing.
    def runTest(self):
        self.assertEqual(str(r.db('db1').table('tbl1').map(lambda x: x)),
                            "r.db('db1').table('tbl1').map(lambda var_1: var_1)")

class TestBatching(TestWithConnection):
    def runTest(self):
        c = r.connect(port=self.port)

        # Test the cursor API when there is exactly mod batch size elements in the result stream
        r.db('test').table_create('t1').run(c)
        t1 = r.table('t1')

        if server_build_dir.find('debug') != -1:
            batch_size = 5
        else:
            batch_size = 1000

        t1.insert([{'id':i} for i in xrange(0, batch_size)]).run(c)
        cursor = t1.run(c)

        # We're going to have to inspect the state of the cursor object to ensure this worked right
        # If this test fails in the future check first if the structure of the object has changed.

        # Only the first chunk (of either 1 or 2) should have loaded
        self.assertEqual(len(cursor.responses), 1)

        # Either the whole stream should have loaded in one batch or the server reserved at least
        # one element in the stream for the second batch.
        if cursor.end_flag:
            self.assertEqual(len(cursor.responses[0].data), batch_size)
        else:
            self.assertLess(len(cursor.responses[0].data), batch_size)

        itr = iter(cursor)
        for i in xrange(0, batch_size - 1):
            itr.next()

        # In both cases now there should at least one element left in the last chunk
        self.assertTrue(cursor.end_flag)
        self.assertGreaterEqual(len(cursor.responses), 1)
        self.assertGreaterEqual(len(cursor.responses[0].response), 1)

class TestGroupWithTimeKey(TestWithConnection):
    def runTest(self):
        c = r.connect(port=self.port)

        r.db('test').table_create('times').run(c)

        time1 = 1375115782.24
        rt1 = r.epoch_time(time1).in_timezone('+00:00')
        dt1 = datetime.datetime.fromtimestamp(time1, r.ast.RqlTzinfo('+00:00'))
        time2 = 1375147296.68
        rt2 = r.epoch_time(time2).in_timezone('+00:00')
        dt2 = datetime.datetime.fromtimestamp(time2, r.ast.RqlTzinfo('+00:00'))

        res = r.table('times').insert({'id':0,'time':rt1}).run(c)
        self.assertEqual(res['inserted'], 1)
        res = r.table('times').insert({'id':1,'time':rt2}).run(c)
        self.assertEqual(res['inserted'], 1)

        expected_row1 = {'id':0,'time':dt1}
        expected_row2 = {'id':1,'time':dt2}

        groups = r.table('times').group('time').coerce_to('array').run(c)
        self.assertEqual(groups, {dt1:[expected_row1],dt2:[expected_row2]})


if __name__ == '__main__':
    print "Running py connection tests"
    suite = unittest.TestSuite()
    loader = unittest.TestLoader()
    suite.addTest(loader.loadTestsFromTestCase(TestNoConnection))
    suite.addTest(loader.loadTestsFromTestCase(TestConnectionDefaultPort))
    suite.addTest(loader.loadTestsFromTestCase(TestTimeout))
    suite.addTest(loader.loadTestsFromTestCase(TestAuthConnection))
    suite.addTest(loader.loadTestsFromTestCase(TestConnection))
    suite.addTest(loader.loadTestsFromTestCase(TestShutdown))
    suite.addTest(TestPrinting())
    suite.addTest(TestBatching())
    suite.addTest(TestGroupWithTimeKey())

    res = unittest.TextTestRunner(verbosity=2).run(suite)
    if not res.wasSuccessful():
        sys.exit(1)
