#!/usr/bin/env python
##
# Tests the driver API for making connections and excercizes the networking code
###

from __future__ import print_function

import datetime, os, random, re, socket, sys, tempfile, threading, time, traceback, unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.realpath(__file__)), os.pardir, os.pardir, "common"))
import driver, utils

try:
    xrange
except NameError:
    xrange = range
try:
    import SocketServer
except:
    import socketserver as SocketServer

# -- import the rethinkdb driver

r = utils.import_python_driver()

# -- get settings

DEFAULT_DRIVER_PORT = 28015

rethinkdb_exe = sys.argv[1] if len(sys.argv) > 1 else utils.find_rethinkdb_executable()
use_default_port = bool(int(sys.argv[2])) if len(sys.argv) > 2 else 0

# -- shared server

sharedServer = None
sharedServerOutput = None
sharedServerHost = None
sharedServerDriverPort = None
if 'RDB_DRIVER_PORT' in os.environ:
    sharedServerDriverPort = int(os.environ['RDB_DRIVER_PORT'])
    if 'RDB_SERVER_HOST' in os.environ:
        sharedServerHost = os.environ['RDB_SERVER_HOST']
    else:
        sharedServerHost = 'localhost'

def checkSharedServer():
    if sharedServerDriverPort is not None:
        conn = r.connect(host=sharedServerHost, port=sharedServerDriverPort)
        if 'test' not in r.db_list().run(conn):
            r.db_create('test').run(conn)

def closeSharedServer():
    global sharedServer, sharedServerOutput, sharedServerHost, sharedServerDriverPort

    if sharedServer is not None:
        try:
            sharedServer.close()
        except Exception as e:
            sys.stderr.write('Got error while shutting down server: %s' % str(e))
    sharedServer = None
    sharedServerOutput = None
    sharedServerHost = None
    sharedServerDriverPort = None


# == Test Base Classes

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
        except Exception as e:
            self.assertTrue(isinstance(e, exception), '%s expected to raise %s but instead raised %s: %s\n%s' % (repr(callable_func), repr(exception), e.__class__.__name__, str(e), traceback.format_exc()))
            self.assertTrue(re.search(regexp, str(e)), '%s did not raise the expected message "%s", but rather: %s' % (repr(callable_func), str(regexp), str(e)))
        else:
            self.fail('%s failed to raise a %s' % (repr(callable_func), repr(exception)))

class TestWithConnection(TestCaseCompatible):

    port = None
    server = None
    serverOutput = None

    def setUp(self):
        global sharedServer, sharedServerOutput, sharedServerHost, sharedServerDriverPort

        if sharedServer is not None:
            try:
                sharedServer.check()
            except Exception:
                # ToDo: figure out how to blame the last test
                closeSharedServer()

        if sharedServerDriverPort is None:
            sharedServerOutput = tempfile.NamedTemporaryFile('w+')
            sharedServer = driver.Process(executable_path=rethinkdb_exe, console_output=sharedServerOutput, wait_until_ready=True)
            sharedServerHost = sharedServer.host
            sharedServerDriverPort = sharedServer.driver_port

        # - insure we are ready

        checkSharedServer()

    def tearDown(self):
        global sharedServer, sharedServerOutput, sharedServerHost, sharedServerDriverPort

        if sharedServerDriverPort is not None:
            try:
                checkSharedServer()
            except Exception:
                closeSharedServer()
                raise # ToDo: figure out how to best give the server log

# == Test Classes

class TestNoConnection(TestCaseCompatible):

    # No servers started yet so this should fail
    def test_connect(self):
        if not use_default_port:
            self.skipTest("Not testing default port")
            return # in case we fell back on replacement_skip
        self.assertRaisesRegexp(r.ReqlDriverError, "Could not connect to localhost:%d." % DEFAULT_DRIVER_PORT, r.connect)

    def test_connect_port(self):
        port = utils.get_avalible_port()
        self.assertRaisesRegexp(r.ReqlDriverError, "Could not connect to localhost:%d." % port, r.connect, port=port)

    def test_connect_host(self):
        if not use_default_port:
            self.skipTest("Not testing default port")
            return # in case we fell back on replacement_skip
        self.assertRaisesRegexp(
            r.ReqlDriverError, "Could not connect to 0.0.0.0:%d." % DEFAULT_DRIVER_PORT, r.connect, host="0.0.0.0")

    def test_connect_timeout(self):
        '''Test that we get a ReQL error if we connect to a non-responsive port'''
        useSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        useSocket.bind(('localhost', 0))
        useSocket.listen(0)

        host, port = useSocket.getsockname()

        try:
            self.assertRaisesRegexp(r.ReqlTimeoutError,
                "Could not connect to %s:%d, operation timed out." % (host, port),
                r.connect, host=host, port=port, timeout=2)
        finally:
            useSocket.close()

    def test_connect_host(self):
        port = utils.get_avalible_port()
        self.assertRaisesRegexp(r.ReqlDriverError, "Could not connect to 0.0.0.0:%d." % port, r.connect, host="0.0.0.0", port=port)

    def test_empty_run(self):
        # Test the error message when we pass nothing to run and didn't call `repl`
        self.assertRaisesRegexp(r.ReqlDriverError, "RqlQuery.run must be given a connection to run on.", r.expr(1).run)

    def test_auth_key(self):
        # Test that everything still doesn't work even with an auth key
        if not use_default_port:
            self.skipTest("Not testing default port")
            return # in case we fell back on replacement_skip
        self.assertRaisesRegexp(r.ReqlDriverError, 'Could not connect to 0.0.0.0:%d."' % DEFAULT_DRIVER_PORT, r.connect, host="0.0.0.0", port=DEFAULT_DRIVER_PORT, auth_key="hunter2")

class TestPrivateServer(TestCaseCompatible):

    server = None
    serverConsoleOutput = None

    useDefaultPort = False
    host = None
    port = None

    authKey = None

    @classmethod
    def setUp(cls):
        if cls.server is not None:
            try:
                cls.server.check()
            except Exception:
                cls.server = None
        if cls.server is None:
            port = str(DEFAULT_DRIVER_PORT) if cls.useDefaultPort else '0'
            cls.serverConsoleOutput = tempfile.NamedTemporaryFile('w+')
            cls.server = driver.Process(executable_path=rethinkdb_exe, console_output=cls.serverConsoleOutput, wait_until_ready=True, extra_options=['--driver-port', port])
            cls.host = cls.server.host
            cls.port = cls.server.driver_port

            if cls.authKey is not None:
                conn = r.connect(host=cls.server.host, port=cls.server.driver_port)
                result = r.db('rethinkdb').table('cluster_config').get('auth').update({'auth_key':cls.authKey}).run(conn)
                if result != {'skipped': 0, 'deleted': 0, 'unchanged': 0, 'errors': 0, 'replaced': 1, 'inserted': 0}:
                    cls.server = None
                    raise Exception('Unable to set authkey, got: %s' % str(result))

    @classmethod
    def tearDown(cls):
        if cls.server is not None:
            try:
                cls.server.check()
            except Exception as e:
                try:
                    cls.server.close()
                except Exception:
                    pass
                cls.server = None
                raise

class TestConnectionDefaultPort(TestPrivateServer):

    useDefaultPort = True

    def setUp(self):
        if not use_default_port:
            self.skipTest("Not testing default port")
            return # in case we fell back on replacement_skip
        super(TestConnectionDefaultPort, self).setUp()

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
        conn = r.connect(host='localhost', port=DEFAULT_DRIVER_PORT)
        conn.reconnect()

    def test_connect_port(self):
        if not use_default_port:
            return
        conn = r.connect(port=DEFAULT_DRIVER_PORT)
        conn.reconnect()

    def test_connect_wrong_auth(self):
        if not use_default_port:
            return
        self.assertRaisesRegexp(
            r.ReqlAuthError,
            "Could not connect to %s:%d, incorrect authentication key." % (self.host, self.port),
            r.connect,
            auth_key="hunter2")

class TestAuthConnection(TestPrivateServer):

    incorrectAuthMessage = "Could not connect to %s:%d, incorrect authentication key."
    authKey = 'hunter2'

    def test_connect_no_auth(self):
        self.assertRaisesRegexp(r.ReqlAuthError, self.incorrectAuthMessage % (self.host, self.port), r.connect, port=self.port)

    def test_connect_wrong_auth(self):
        self.assertRaisesRegexp(
            r.ReqlAuthError,
            self.incorrectAuthMessage % (self.host, self.port),
            r.connect, port=self.port, auth_key="")
        self.assertRaisesRegexp(
            r.ReqlAuthError,
            self.incorrectAuthMessage % (self.host, self.port),
            r.connect, port=self.port, auth_key="hunter3")
        self.assertRaisesRegexp(
            r.ReqlAuthError,
            self.incorrectAuthMessage % (self.host, self.port),
            r.connect, port=self.port, auth_key="hunter22")

    def test_connect_long_auth(self):
        long_key = str("k") * 2049
        not_long_key = str("k") * 2048

        self.assertRaisesRegexp(
            r.ReqlDriverError, "Server dropped connection with message: \"ERROR: Client provided an authorization key that is too long.\"",
            r.connect, port=self.port, auth_key=long_key)

        self.assertRaisesRegexp(
            r.ReqlDriverError,
            self.incorrectAuthMessage % (self.host, self.port),
            r.connect, port=self.port, auth_key=not_long_key)

    def test_connect_correct_auth(self):
        conn = r.connect(port=self.port, auth_key="hunter2")
        conn.reconnect()

class TestConnection(TestWithConnection):
    def test_connect_close_reconnect(self):
        c = r.connect(host=sharedServerHost, port=sharedServerDriverPort)
        r.expr(1).run(c)
        c.close()
        c.close()
        c.reconnect()
        r.expr(1).run(c)

    def test_connect_close_expr(self):
        c = r.connect(host=sharedServerHost, port=sharedServerDriverPort)
        r.expr(1).run(c)
        c.close()
        self.assertRaisesRegexp(
            r.ReqlDriverError, "Connection is closed.",
            r.expr(1).run, c)

    def test_noreply_wait_waits(self):
        c = r.connect(host=sharedServerHost, port=sharedServerDriverPort)
        t = time.time()
        r.js('while(true);', timeout=0.5).run(c, noreply=True)
        c.noreply_wait()
        duration = time.time() - t
        self.assertGreaterEqual(duration, 0.5)

    def test_close_waits_by_default(self):
        c = r.connect(host=sharedServerHost, port=sharedServerDriverPort)
        t = time.time()
        r.js('while(true);', timeout=0.5).run(c, noreply=True)
        c.close()
        duration = time.time() - t
        self.assertGreaterEqual(duration, 0.5)

    def test_reconnect_waits_by_default(self):
        c = r.connect(host=sharedServerHost, port=sharedServerDriverPort)
        t = time.time()
        r.js('while(true);', timeout=0.5).run(c, noreply=True)
        c.reconnect()
        duration = time.time() - t
        self.assertGreaterEqual(duration, 0.5)

    def test_close_does_not_wait_if_requested(self):
        c = r.connect(host=sharedServerHost, port=sharedServerDriverPort)
        t = time.time()
        r.js('while(true);', timeout=0.5).run(c, noreply=True)
        c.close(noreply_wait=False)
        duration = time.time() - t
        self.assertLess(duration, 0.5)

    def test_reconnect_does_not_wait_if_requested(self):
        c = r.connect(host=sharedServerHost, port=sharedServerDriverPort)
        t = time.time()
        r.js('while(true);', timeout=0.5).run(c, noreply=True)
        c.reconnect(noreply_wait=False)
        duration = time.time() - t
        self.assertLess(duration, 0.5)

    def test_db(self):
        c = r.connect(host=sharedServerHost, port=sharedServerDriverPort)

        if 't1' in r.db('test').table_list().run(c):
            r.db('test').table_drop('t1').run(c)
        r.db('test').table_create('t1').run(c)

        if 'db2' in r.db_list().run(c):
            r.db_drop('db2').run(c)
        r.db_create('db2').run(c)

        if 't2' in r.db('db2').table_list().run(c):
            r.db('db2').table_drop('t2').run(c)
        r.db('db2').table_create('t2').run(c)

        # Default db should be 'test' so this will work
        r.table('t1').run(c)

        # Use a new database
        c.use('db2')
        r.table('t2').run(c)
        self.assertRaisesRegexp(r.ReqlRuntimeError, "Table `db2.t1` does not exist.", r.table('t1').run, c)

        c.use('test')
        r.table('t1').run(c)
        self.assertRaisesRegexp(r.ReqlRuntimeError, "Table `test.t2` does not exist.", r.table('t2').run, c)

        c.close()

        # Test setting the db in connect
        c = r.connect(db='db2', host=sharedServerHost, port=sharedServerDriverPort)
        r.table('t2').run(c)

        self.assertRaisesRegexp(r.ReqlRuntimeError, "Table `db2.t1` does not exist.", r.table('t1').run, c)

        c.close()

        # Test setting the db as a `run` option
        c = r.connect(host=sharedServerHost, port=sharedServerDriverPort)
        r.table('t2').run(c, db='db2')

    def test_outdated_read(self):
        c = r.connect(host=sharedServerHost, port=sharedServerDriverPort)

        if 't1' in r.db('test').table_list().run(c):
            r.db('test').table_drop('t1').run(c)
        r.db('test').table_create('t1').run(c)

        # Use outdated is an option that can be passed to db.table or `run`
        # We're just testing here if the server actually accepts the option.

        r.table('t1', read_mode='outdated').run(c)
        r.table('t1').run(c, read_mode='outdated')

    def test_repl(self):

        # Calling .repl() should set this connection as global state
        # to be used when `run` is not otherwise passed a connection.
        c = r.connect(host=sharedServerHost, port=sharedServerDriverPort).repl()

        r.expr(1).run()

        c.repl() # is idempotent

        r.expr(1).run()

        c.close()

        self.assertRaisesRegexp(r.ReqlDriverError, "Connection is closed", r.expr(1).run)

    def test_port_conversion(self):
        c = r.connect(host=sharedServerHost, port=str(sharedServerDriverPort))
        r.expr(1).run(c)
        c.close()

        self.assertRaisesRegexp(r.ReqlDriverError, "Could not convert port abc to an integer.", r.connect, port='abc', host=sharedServerHost)

class TestShutdown(TestWithConnection):

    def setUp(self):
        if sharedServer is None:
            closeSharedServer() # we need to be able to kill the server, so can't use one from outside
        super(TestShutdown, self).setUp()

    def test_shutdown(self):
        c = r.connect(host=sharedServerHost, port=sharedServerDriverPort)
        r.expr(1).run(c)

        closeSharedServer()
        time.sleep(0.2)

        self.assertRaisesRegexp(r.ReqlDriverError, "Connection is closed.", r.expr(1).run, c)

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

        c = r.connect(host=sharedServerHost, port=sharedServerDriverPort)

        if 't1' in r.db('test').table_list().run(c):
            r.db('test').table_drop('t1').run(c)
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
        print("Random seed: " + str(rseed), end=' ')
        sys.stdout.flush()

        points = []
        for i in xrange(0, point_count):
            points.append({'geo':r.point(random.uniform(-180.0, 180.0), random.uniform(-90.0, 90.0))})
        polygons = []
        for i in xrange(0, poly_count):
            # A fairly big circle, so it will cover a large range in the secondary index
            polygons.append({'geo':r.circle([random.uniform(-180.0, 180.0), random.uniform(-90.0, 90.0)], 1000000)})
        t1.insert(points).run(c)
        t1.insert(polygons).run(c)

        # Check that the results are actually lazy at least some of the time
        # While the test is randomized, chances are extremely high to get a lazy result at least once.
        seen_lazy = False

        for i in xrange(0, get_tries):
            query_circle = r.circle([random.uniform(-180.0, 180.0), random.uniform(-90.0, 90.0)], 8000000);
            reference = t1.filter(r.row['geo'].intersects(query_circle)).coerce_to("ARRAY").run(c)
            cursor = t1.get_intersecting(query_circle, index='geo').run(c, max_batch_rows=batch_size)
            if cursor.error is None:
                seen_lazy = True

            itr = iter(cursor)
            while len(reference) > 0:
                row = next(itr)
                self.assertEqual(reference.count(row), 1)
                reference.remove(row)
            self.assertRaises(r.ReqlCursorEmpty, lambda: next(itr))

        self.assertTrue(seen_lazy)

        r.db('test').table_drop('t1').run(c)

class TestBatching(TestWithConnection):
    def runTest(self):
        c = r.connect(host=sharedServerHost, port=sharedServerDriverPort)

        # Test the cursor API when there is exactly mod batch size elements in the result stream
        if 't1' in r.db('test').table_list().run(c):
            r.db('test').table_drop('t1').run(c)
        r.db('test').table_create('t1').run(c)
        t1 = r.table('t1')

        batch_size = 3
        count = 500

        ids = set(xrange(0, count))

        t1.insert([{'id':i} for i in ids]).run(c)
        cursor = t1.run(c, max_batch_rows=batch_size)

        itr = iter(cursor)
        for i in xrange(0, count - 1):
            row = next(itr)
            ids.remove(row['id'])

        self.assertEqual(next(itr)['id'], ids.pop())
        self.assertRaises(r.ReqlCursorEmpty, lambda: next(itr))
        r.db('test').table_drop('t1').run(c)

class TestGroupWithTimeKey(TestWithConnection):
    def runTest(self):
        c = r.connect(host=sharedServerHost, port=sharedServerDriverPort)

        if 'times' in r.db('test').table_list().run(c):
            r.db('test').table_drop('times').run(c)
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


class TestSuccessAtomFeed(TestWithConnection):
    def runTest(self):
        c = r.connect(host=sharedServerHost, port=sharedServerDriverPort)

        from rethinkdb import ql2_pb2 as p

        if 'success_atom_feed' in r.db('test').table_list().run(c):
            r.db('test').table_drop('success_atom_feed').run(c)
        r.db('test').table_create('success_atom_feed').run(c)
        t1 = r.db('test').table('success_atom_feed')

        res = t1.insert({'id': 0, 'a': 16}).run(c)
        self.assertEqual(res['inserted'], 1)
        res = t1.insert({'id': 1, 'a': 31}).run(c)
        self.assertEqual(res['inserted'], 1)

        t1.index_create('a', lambda x: x['a']).run(c)
        t1.index_wait('a').run(c)

        changes = t1.get(0).changes(include_initial=True).run(c)
        self.assertTrue(changes.error is None)
        self.assertEqual(len(changes.items), 1)


if __name__ == '__main__':
    print("Running py connection tests")
    suite = unittest.TestSuite()
    loader = unittest.TestLoader()
    suite.addTest(loader.loadTestsFromTestCase(TestNoConnection))
    if use_default_port:
        suite.addTest(loader.loadTestsFromTestCase(TestConnectionDefaultPort))
    suite.addTest(loader.loadTestsFromTestCase(TestAuthConnection))
    suite.addTest(loader.loadTestsFromTestCase(TestConnection))
    suite.addTest(TestPrinting())
    suite.addTest(TestBatching())
    suite.addTest(TestGetIntersectingBatching())
    suite.addTest(TestGroupWithTimeKey())
    suite.addTest(TestSuccessAtomFeed())
    suite.addTest(loader.loadTestsFromTestCase(TestShutdown))

    res = unittest.TextTestRunner(verbosity=2).run(suite)

    serverClosedCleanly = True
    try:
        if sharedServer is not None:
            sharedServer.check_and_stop()
    except Exception as e:
        serverClosedCleanly = False
        sys.stderr.write('The server did not close cleanly after testing: %s' % str(e))

    if not res.wasSuccessful() or not serverClosedCleanly:
        sys.exit(1)
