#!/usr/bin/env python
##
# Tests the driver API for making connections and excercizes the networking code
###

from __future__ import print_function

import datetime, inspect, os, re, socket, sys, threading, unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.realpath(__file__)), os.pardir))
import test_util

try:
    xrange
except NameError:
    xrange = range
try:
    import SocketServer
except:
    import socketserver as SocketServer

# - import the rethinkdb driver

sys.path.insert(0, os.path.join(os.path.dirname(os.path.realpath(__file__)), os.pardir, os.pardir, "common"))
import utils
r = utils.import_python_driver()

# - import it using the 'from rethinkdb import *' form
sys.path.insert(0, os.path.dirname(inspect.getfile(r)))
from rethinkdb import *

import time # overrides the import of rethinkdb.time

if len(sys.argv) > 1:
    server_build_dir = sys.argv[1]
else:
    server_build_dir = utils.latest_build_dir()

use_default_port = 0
if len(sys.argv) > 2:
    use_default_port = bool(int(sys.argv[2]))

class TestCaseCompatible(unittest.TestCase):
    '''Compatibility shim for Python 2.6'''
    
    def __init__(self, *args, **kwargs):
        super(TestCaseCompatible, self).__init__(*args, **kwargs)
        
        if not hasattr(self, 'assertRaisesRegexp'):
            self.assertRaisesRegexp = self.replacement_assertRaisesRegexp
        if not hasattr(self, 'skipTest'):
            self.skipTest = self.replacement_skipTest
        if not hasattr(self, 'assertGreaterEqual'):
            self.assertGreaterEqual = self.replacement_assertGreaterEqual
        if not hasattr(self, 'assertLess'):
            self.assertLess = self.replacement_assertLess
    
    def replacement_assertGreaterEqual(self, greater, lesser):
        if not greater >= lesser:
            raise AssertionError('%s not greater than or equal to %s' % (greater, lesser))
    
    def replacement_assertLess(self, lesser, greater):
        if not greater > lesser:
            raise AssertionError('%s not less than %s' % (lesser, greater))
    
    def replacement_skipTest(self, message):
        sys.stderr.write("%s " % message)
    
    def replacement_assertRaisesRegexp(self, exception, regexp, callable_func, *args, **kwds):
        try:
            callable_func(*args, **kwds)
            self.fail('%s failed to raise a %s' % (repr(callable_func), repr(exception)))
        except Exception as e:
            self.assertTrue(isinstance(e, exception), '%s expected to raise %s but instead raised %s: %s' % (repr(callable_func), repr(exception), e.__class__.__name__, str(e)))
            self.assertTrue(re.search(regexp, str(e)), '%s did not raise the expected message "%s", but rather: %s' % (repr(callable_func), str(regexp), str(e)))

class TestNoConnection(TestCaseCompatible):
    # No servers started yet so this should fail
    def test_connect(self):
        if not use_default_port:
            self.skipTest("Not testing default port")
            return # in case we fell back on replacement_skip
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
            return # in case we fell back on replacement_skip
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
            return # in case we fell back on replacement_skip
        self.assertRaisesRegexp(
            RqlDriverError, 'Could not connect to 0.0.0.0:28015."',
            r.connect, host="0.0.0.0", port=28015, auth_key="hunter2")

class TestConnectionDefaultPort(TestCaseCompatible):
    
    servers = None
    
    def setUp(self):
        if not use_default_port:
            self.skipTest("Not testing default port")
            return # in case we fell back on replacement_skip
        self.servers = test_util.RethinkDBTestServers(4, server_build_dir=server_build_dir, use_default_port=use_default_port)
        self.servers.__enter__()

    def tearDown(self):
        if self.servers is not None:
            self.servers.__exit__(None, None, None)

    def test_connect(self):
        if not use_default_port:
            return
        conn = r.connect()
        conn.reconnect()

    def test_connect_host(self):
        if not use_default_port:
            return
        conn = r.connect(host='localhost')
        conn.reconnect()

    def test_connect_host_port(self):
        if not use_default_port:
            return
        conn = r.connect(host='localhost', port=28015)
        conn.reconnect()

    def test_connect_port(self):
        if not use_default_port:
            return
        conn = r.connect(port=28015)
        conn.reconnect()

    def test_connect_wrong_auth(self):
        if not use_default_port:
            return
        self.assertRaisesRegexp(
            RqlDriverError, "Server dropped connection with message: \"ERROR: Incorrect authorization key.\"",
            r.connect, auth_key="hunter2")

class BlackHoleRequestHandler(SocketServer.BaseRequestHandler):
    def handle(self):
        time.sleep(1)

class ThreadedBlackHoleServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
    pass

class TestTimeout(TestCaseCompatible):
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

class TestAuthConnection(TestCaseCompatible):

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

class TestWithConnection(TestCaseCompatible):

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
        t = time.time()
        r.js('while(true);', timeout=0.5).run(c, noreply=True)
        c.noreply_wait()
        duration = time.time() - t
        self.assertGreaterEqual(duration, 0.5)

    def test_close_waits_by_default(self):
        c = r.connect(port=self.port)
        t = time.time()
        r.js('while(true);', timeout=0.5).run(c, noreply=True)
        c.close()
        duration = time.time() - t
        self.assertGreaterEqual(duration, 0.5)

    def test_reconnect_waits_by_default(self):
        c = r.connect(port=self.port)
        t = time.time()
        r.js('while(true);', timeout=0.5).run(c, noreply=True)
        c.reconnect()
        duration = time.time() - t
        self.assertGreaterEqual(duration, 0.5)

    def test_close_does_not_wait_if_requested(self):
        c = r.connect(port=self.port)
        t = time.time()
        r.js('while(true);', timeout=0.5).run(c, noreply=True)
        c.close(noreply_wait=False)
        duration = time.time() - t
        self.assertLess(duration, 0.5)

    def test_reconnect_does_not_wait_if_requested(self):
        c = r.connect(port=self.port)
        t = time.time()
        r.js('while(true);', timeout=0.5).run(c, noreply=True)
        c.reconnect(noreply_wait=False)
        duration = time.time() - t
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
        time.sleep(0.2)
        self.assertRaisesRegexp(
            r.RqlDriverError, "Connection is closed.",
            r.expr(1).run, c)


# This doesn't really have anything to do with connections but it'll go
# in here for the time being.
class TestPrinting(TestCaseCompatible):

    # Just test that RQL queries support __str__ using the pretty printer.
    # An exhaustive test of the pretty printer would be, well, exhausting.
    def runTest(self):
        self.assertEqual(str(r.db('db1').table('tbl1').map(lambda x: x)),
                            "r.db('db1').table('tbl1').map(lambda var_1: var_1)")

# Another non-connection connection test. It's to test that get_intersecting()
# batching works properly.
class TestGetIntersectingBatching(TestWithConnection):
    def runTest(self):
        import random # importing here to avoid issue #2343

        c = r.connect(port=self.port)

        r.db('test').table_create('t1').run(c)
        t1 = r.db('test').table('t1')

        t1.index_create('geo', geo=True).run(c)
        t1.index_wait('geo').run(c)

        batch_size = 3
        point_count = 500
        poly_count = 500
        get_tries = 10

        # Insert a couple of random points, so we get a well distributed range of
        # secondary keys. Also insert a couple of large-ish polygons, so we can
        # test filtering of duplicates on the server.
        rseed = random.getrandbits(64)
        random.seed(rseed)
        print("Random seed: " + str(rseed))
        points = []
        for i in range(0, point_count):
            points.append({'geo':r.point(random.uniform(-90.0, 90.0), random.uniform(-180.0, 180.0))})
        polygons = []
        for i in range(0, poly_count):
            # A fairly big circle, so it will cover a large range in the secondary index
            polygons.append({'geo':r.circle([random.uniform(-90.0, 90.0), random.uniform(-180.0, 180.0)], 1000000)})
        t1.insert(points).run(c)
        t1.insert(polygons).run(c)

        # Check that the results are actually lazy at least some of the time
        # While the test is randomized, chances are extremely high to get a lazy result at least once.
        seen_lazy = False

        for i in range(0, get_tries):
            query_circle = r.circle([random.uniform(-90.0, 90.0), random.uniform(-180.0, 180.0)], 8000000);
            reference = t1.filter(r.row['geo'].intersects(query_circle)).coerce_to("ARRAY").run(c)
            cursor = t1.get_intersecting(query_circle, index='geo').run(c, batch_conf={'max_els':batch_size})
            if not cursor.end_flag:
                seen_lazy = True

            itr = iter(cursor)
            while len(reference) > 0:
                row = next(itr)
                self.assertEqual(reference.count(row), 1)
                reference.remove(row)
            self.assertRaises(StopIteration, lambda: next(itr))
            self.assertTrue(cursor.end_flag)

        self.assertTrue(seen_lazy)

        r.db('test').table_drop('t1').run(c)

class TestBatching(TestWithConnection):
    def runTest(self):
        c = r.connect(port=self.port)

        # Test the cursor API when there is exactly mod batch size elements in the result stream
        r.db('test').table_create('t1').run(c)
        t1 = r.table('t1')

        batch_size = 3
        count = 500

        ids = set(range(0, count))

        t1.insert([{'id':i} for i in ids]).run(c)
        cursor = t1.run(c, batch_conf={'max_els':batch_size})

        itr = iter(cursor)
        for i in xrange(0, count - 1):
            row = next(itr)
            ids.remove(row['id'])

        self.assertEqual(next(itr)['id'], ids.pop())
        self.assertRaises(StopIteration, lambda: next(itr))
        self.assertTrue(cursor.end_flag)
        r.db('test').table_drop('t1').run(c)

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

        res = r.table('times').insert({'id':0, 'time':rt1}).run(c)
        self.assertEqual(res['inserted'], 1)
        res = r.table('times').insert({'id':1, 'time':rt2}).run(c)
        self.assertEqual(res['inserted'], 1)

        expected_row1 = {'id':0, 'time':dt1}
        expected_row2 = {'id':1, 'time':dt2}

        groups = r.table('times').group('time').coerce_to('array').run(c)
        self.assertEqual(groups, {dt1:[expected_row1], dt2:[expected_row2]})


if __name__ == '__main__':
    print("Running py connection tests")
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
    suite.addTest(TestGetIntersectingBatching())
    suite.addTest(TestGroupWithTimeKey())

    res = unittest.TextTestRunner(verbosity=2).run(suite)
    if not res.wasSuccessful():
        sys.exit(1)
