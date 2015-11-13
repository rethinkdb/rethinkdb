#!/usr/bin/env python
##
# Tests the driver API for making connections and exercizes the
# networking code
###

from __future__ import print_function

import asyncio
import datetime
import os
import random
import re
import socket
import sys
import tempfile
import time
import traceback
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.realpath(__file__)),
                                os.pardir, os.pardir, "common"))
import driver
import utils

try:
    xrange
except NameError:
    xrange = range

# -- import the rethinkdb driver

r = utils.import_python_driver()

# -- use asyncio subdriver

r.set_loop_type("asyncio")

# -- get settings

DEFAULT_DRIVER_PORT = 28015

rethinkdb_exe = (sys.argv[1]
                 if len(sys.argv) > 1
                 else utils.find_rethinkdb_executable())
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


@asyncio.coroutine
def checkSharedServer():
    if sharedServerDriverPort is not None:
        conn = yield from r.connect(host=sharedServerHost,
                               port=sharedServerDriverPort)
        if 'test' not in (yield from r.db_list().run(conn)):
            yield from r.db_create('test').run(conn)


@asyncio.coroutine
def closeSharedServer():
    global sharedServer, sharedServerOutput, sharedServerHost, \
        sharedServerDriverPort

    if sharedServer is not None:
        try:
            sharedServer.close()
        except Exception as e:
            sys.stderr.write('Got error while shutting down server: %s'
                             % str(e))
    sharedServer = None
    sharedServerOutput = None
    sharedServerHost = None
    sharedServerDriverPort = None


def just_do(coroutine, *args, **kwargs):
    asyncio.get_event_loop().run_until_complete(coroutine(*args, **kwargs))


# == Test Base Classes

class TestCaseCompatible(unittest.TestCase):
    # can't use standard TestCase run here because async.
    def run(self, result=None):
        return just_do(self.arun, result)

    @asyncio.coroutine
    def setUp(self):
        return None

    @asyncio.coroutine
    def tearDown(self):
        return None

    @asyncio.coroutine
    def arun(self, result=None):
        if result is None:
            result = self.defaultTestResult()
        result.startTest(self)
        testMethod = getattr(self, self._testMethodName)

        try:
            try:
                yield from self.setUp()
            except KeyboardInterrupt:
                raise
            except:
                result.addError(self, sys.exc_info())
                return

            ok = False
            try:
                yield from testMethod()
                ok = True
            except self.failureException:
                result.addFailure(self, sys.exc_info())
            except KeyboardInterrupt:
                raise
            except:
                result.addError(self, sys.exc_info())

            try:
                yield from self.tearDown()
            except KeyboardInterrupt:
                raise
            except:
                result.addError(self, sys.exc_info())
                ok = False
            if ok:
                result.addSuccess(self)
        finally:
            result.stopTest(self)


class TestWithConnection(TestCaseCompatible):
    port = None
    server = None
    serverOutput = None
    ioloop = None

    @asyncio.coroutine
    def setUp(self):
        global sharedServer, sharedServerOutput, sharedServerHost, \
            sharedServerDriverPort

        if sharedServer is not None:
            try:
                yield from sharedServer.check()
            except Exception:
                # ToDo: figure out how to blame the last test
                yield from closeSharedServer()

        if sharedServerDriverPort is None:
            sharedServerOutput = tempfile.NamedTemporaryFile('w+')
            sharedServer = driver.Process(executable_path=rethinkdb_exe,
                                          console_output=sharedServerOutput,
                                          wait_until_ready=True)
            sharedServerHost = sharedServer.host
            sharedServerDriverPort = sharedServer.driver_port

        # - insure we are ready

        yield from checkSharedServer()

    @asyncio.coroutine
    def tearDown(self):
        global sharedServer, sharedServerOutput, sharedServerHost, \
            sharedServerDriverPort

        if sharedServerDriverPort is not None:
            try:
                yield from checkSharedServer()
            except Exception:
                yield from closeSharedServer()
                raise  # ToDo: figure out how to best give the server log

# == Test Classes


class TestNoConnection(TestCaseCompatible):
    # No servers started yet so this should fail
    @asyncio.coroutine
    def test_connect(self):
        if not use_default_port:
            return None  # skipTest will not raise in this environment
        with self.assertRaisesRegexp(r.ReqlDriverError,
                                           "Could not connect to localhost:%d."
                                           % DEFAULT_DRIVER_PORT):
            yield from r.connect()

    @asyncio.coroutine
    def test_connect_port(self):
        port = utils.get_avalible_port()
        with self.assertRaisesRegexp(r.ReqlDriverError,
                                           "Could not connect to localhost:%d."
                                           % port):
            yield from r.connect(port=port)

    @asyncio.coroutine
    def test_connect_host(self):
        if not use_default_port:
            return None  # skipTest will not raise in this environment
        with self.assertRaisesRegexp(r.ReqlDriverError,
                                           "Could not connect to 0.0.0.0:%d."
                                           % DEFAULT_DRIVER_PORT):
            yield from r.connect(host="0.0.0.0")

    @asyncio.coroutine
    def test_connect_timeout(self):
        # Test that we get a ReQL error if we connect to a
        # non-responsive port
        useSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        useSocket.bind(('localhost', 0))
        useSocket.listen(0)

        host, port = useSocket.getsockname()

        try:
            with self.assertRaisesRegexp(r.ReqlDriverError,
                                               "Connection interrupted during"
                                               " handshake with %s:%d. "
                                               "Error: Operation timed out."
                                               % (host, port)):
                yield from r.connect(host=host, port=port, timeout=2)
        finally:
            useSocket.close()

    @asyncio.coroutine
    def test_empty_run(self):
        # Test the error message when we pass nothing to run and
        # didn't call `repl`
        with self.assertRaisesRegexp(r.ReqlDriverError,
                                "RqlQuery.run must be given"
                                " a connection to run on."):
             r.expr(1).run()

    @asyncio.coroutine
    def test_auth_key(self):
        # Test that everything still doesn't work even with an auth key
        if not use_default_port:
            return None  # skipTest will not raise in this environment
        with self.assertRaisesRegexp(r.ReqlDriverError,
                                           'Could not connect to 0.0.0.0:%d."'
                                           % DEFAULT_DRIVER_PORT):
            yield from r.connect(host="0.0.0.0", port=DEFAULT_DRIVER_PORT,
                                 auth_key="hunter2")


class TestConnection(TestWithConnection):
    @asyncio.coroutine
    def test_connect_close_reconnect(self):
        c = yield from r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        yield from r.expr(1).run(c)
        yield from c.close()
        yield from c.close()
        yield from c.reconnect()
        yield from r.expr(1).run(c)

    @asyncio.coroutine
    def test_connect_close_expr(self):
        c = yield from r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        yield from r.expr(1).run(c)
        yield from c.close()
        with self.assertRaisesRegexp(r.ReqlDriverError, "Connection is closed."):
            yield from r.expr(1).run(c)

    @asyncio.coroutine
    def test_noreply_wait_waits(self):
        c = yield from r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        t = time.time()
        yield from r.js('while(true);', timeout=0.5).run(c, noreply=True)
        yield from c.noreply_wait()
        duration = time.time() - t
        self.assertGreaterEqual(duration, 0.5)

    @asyncio.coroutine
    def test_close_waits_by_default(self):
        c = yield from r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        t = time.time()
        yield from r.js('while(true);', timeout=0.5).run(c, noreply=True)
        yield from c.close()
        duration = time.time() - t
        self.assertGreaterEqual(duration, 0.5)

    @asyncio.coroutine
    def test_reconnect_waits_by_default(self):
        c = yield from r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        t = time.time()
        yield from r.js('while(true);', timeout=0.5).run(c, noreply=True)
        yield from c.reconnect()
        duration = time.time() - t
        self.assertGreaterEqual(duration, 0.5)

    @asyncio.coroutine
    def test_close_does_not_wait_if_requested(self):
        c = yield from r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        t = time.time()
        yield from r.js('while(true);', timeout=0.5).run(c, noreply=True)
        yield from c.close(noreply_wait=False)
        duration = time.time() - t
        self.assertLess(duration, 0.5)

    @asyncio.coroutine
    def test_reconnect_does_not_wait_if_requested(self):
        c = yield from r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        t = time.time()
        yield from r.js('while(true);', timeout=0.5).run(c, noreply=True)
        yield from c.reconnect(noreply_wait=False)
        duration = time.time() - t
        self.assertLess(duration, 0.5)

    @asyncio.coroutine
    def test_db(self):
        c = yield from r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)

        if 't1' in (yield from r.db('test').table_list().run(c)):
            yield from r.db('test').table_drop('t1').run(c)
        yield from r.db('test').table_create('t1').run(c)

        if 'db2' in (yield from r.db_list().run(c)):
            yield from r.db_drop('db2').run(c)
        yield from r.db_create('db2').run(c)

        if 't2' in (yield from r.db('db2').table_list().run(c)):
            yield from r.db('db2').table_drop('t2').run(c)
        yield from r.db('db2').table_create('t2').run(c)

        # Default db should be 'test' so this will work
        yield from r.table('t1').run(c)

        # Use a new database
        c.use('db2')
        yield from r.table('t2').run(c)
        with self.assertRaisesRegexp(r.ReqlRuntimeError,
                                           "Table `db2.t1` does not exist."):
            yield from r.table('t1').run(c)

        c.use('test')
        yield from r.table('t1').run(c)
        with self.assertRaisesRegexp(r.ReqlRuntimeError,
                                           "Table `test.t2` does not exist."):
            yield from r.table('t2').run(c)

        yield from c.close()

        # Test setting the db in connect
        c = yield from r.connect(db='db2', host=sharedServerHost,
                            port=sharedServerDriverPort)
        yield from r.table('t2').run(c)

        with self.assertRaisesRegexp(r.ReqlRuntimeError,
                                           "Table `db2.t1` does not exist."):
            yield from r.table('t1').run(c)

        yield from c.close()

        # Test setting the db as a `run` option
        c = yield from r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        yield from r.table('t2').run(c, db='db2')

    @asyncio.coroutine
    def test_outdated_read(self):
        c = yield from r.connect(host=sharedServerHost,
                                 port=sharedServerDriverPort)

        if 't1' in (yield from r.db('test').table_list().run(c)):
            yield from r.db('test').table_drop('t1').run(c)
        yield from r.db('test').table_create('t1').run(c)

        # Use outdated is an option that can be passed to db.table or `run`
        # We're just testing here if the server actually accepts the option.

        yield from r.table('t1', read_mode='outdated').run(c)
        yield from r.table('t1').run(c, read_mode='outdated')

    @asyncio.coroutine
    def test_repl(self):
        # Calling .repl() should set this connection as global state
        # to be used when `run` is not otherwise passed a connection.
        c = (yield from r.connect(host=sharedServerHost,
                             port=sharedServerDriverPort)).repl()

        yield from r.expr(1).run()

        c.repl()                # is idempotent

        yield from r.expr(1).run()

        yield from c.close()

        with self.assertRaisesRegexp(r.ReqlDriverError, "Connection is closed."):
            yield from r.expr(1).run()

    @asyncio.coroutine
    def test_port_conversion(self):
        c = yield from r.connect(host=sharedServerHost,
                            port=str(sharedServerDriverPort))
        yield from r.expr(1).run(c)
        yield from c.close()

        with self.assertRaisesRegexp(r.ReqlDriverError,
                               "Could not convert port abc to an integer."):
            yield from r.connect(port='abc', host=sharedServerHost)


class TestShutdown(TestWithConnection):
    @asyncio.coroutine
    def setUp(self):
        if sharedServer is None:
            # we need to be able to kill the server, so can't use one
            # from outside
            yield from closeSharedServer()
        yield from super(TestShutdown, self).setUp()

    @asyncio.coroutine
    def test_shutdown(self):
        c = yield from r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        yield from r.expr(1).run(c)

        yield from closeSharedServer()
        yield from asyncio.sleep(0.2)

        with self.assertRaisesRegexp(r.ReqlDriverError,
                               "Connection is closed."):
            yield from r.expr(1).run(c)


# Another non-connection connection test. It's to test that get_intersecting()
# batching works properly.
class TestGetIntersectingBatching(TestWithConnection):
    @asyncio.coroutine
    def runTest(self):
        c = yield from r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)

        if 't1' in (yield from r.db('test').table_list().run(c)):
            yield from r.db('test').table_drop('t1').run(c)
        yield from r.db('test').table_create('t1').run(c)
        t1 = r.db('test').table('t1')

        yield from t1.index_create('geo', geo=True).run(c)
        yield from t1.index_wait('geo').run(c)

        batch_size = 3
        point_count = 500
        poly_count = 500
        get_tries = 10

        # Insert a couple of random points, so we get a well
        # distributed range of secondary keys. Also insert a couple of
        # large-ish polygons, so we can test filtering of duplicates
        # on the server.
        rseed = random.getrandbits(64)
        random.seed(rseed)
        print("Random seed: " + str(rseed), end=' ')
        sys.stdout.flush()

        points = []
        for i in xrange(0, point_count):
            points.append({'geo': r.point(random.uniform(-180.0, 180.0),
                                          random.uniform(-90.0, 90.0))})
        polygons = []
        for i in xrange(0, poly_count):
            # A fairly big circle, so it will cover a large range in
            # the secondary index
            polygons.append({'geo': r.circle([random.uniform(-180.0, 180.0),
                                              random.uniform(-90.0, 90.0)],
                                             1000000)})
        yield from t1.insert(points).run(c)
        yield from t1.insert(polygons).run(c)

        # Check that the results are actually lazy at least some of
        # the time While the test is randomized, chances are extremely
        # high to get a lazy result at least once.
        seen_lazy = False

        for i in xrange(0, get_tries):
            query_circle = r.circle([random.uniform(-180.0, 180.0),
                                     random.uniform(-90.0, 90.0)], 8000000)
            reference = yield from t1.filter(r.row['geo'].intersects(query_circle))\
                                .coerce_to("ARRAY").run(c)
            cursor = yield from t1.get_intersecting(query_circle, index='geo')\
                             .run(c, max_batch_rows=batch_size)
            if cursor.error is None:
                seen_lazy = True

            while len(reference) > 0:
                row = yield from cursor.next()
                self.assertEqual(reference.count(row), 1)
                reference.remove(row)
            with self.assertRaises(r.ReqlCursorEmpty):
                yield from cursor.next()

        self.assertTrue(seen_lazy)

        yield from r.db('test').table_drop('t1').run(c)


class TestBatching(TestWithConnection):
    @asyncio.coroutine
    def runTest(self):
        c = yield from r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)

        # Test the cursor API when there is exactly mod batch size
        # elements in the result stream
        if 't1' in (yield from r.db('test').table_list().run(c)):
            yield from r.db('test').table_drop('t1').run(c)
        yield from r.db('test').table_create('t1').run(c)
        t1 = r.table('t1')

        batch_size = 3
        count = 500

        ids = set(xrange(0, count))

        yield from t1.insert([{'id': i} for i in ids]).run(c)
        cursor = yield from t1.run(c, max_batch_rows=batch_size)

        for i in xrange(0, count - 1):
            row = yield from cursor.next()
            self.assertTrue(row['id'] in ids)
            ids.remove(row['id'])

        self.assertEqual((yield from cursor.next())['id'], ids.pop())
        with self.assertRaises(r.ReqlCursorEmpty):
            yield from cursor.next()
        yield from r.db('test').table_drop('t1').run(c)


class TestGroupWithTimeKey(TestWithConnection):
    @asyncio.coroutine
    def runTest(self):
        c = yield from r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)

        if 'times' in (yield from r.db('test').table_list().run(c)):
            yield from r.db('test').table_drop('times').run(c)
        yield from r.db('test').table_create('times').run(c)

        time1 = 1375115782.24
        rt1 = r.epoch_time(time1).in_timezone('+00:00')
        dt1 = datetime.datetime.fromtimestamp(time1, r.ast.RqlTzinfo('+00:00'))
        time2 = 1375147296.68
        rt2 = r.epoch_time(time2).in_timezone('+00:00')
        dt2 = datetime.datetime.fromtimestamp(time2, r.ast.RqlTzinfo('+00:00'))

        res = yield from r.table('times').insert({'id': 0, 'time': rt1}).run(c)
        self.assertEqual(res['inserted'], 1)
        res = yield from r.table('times').insert({'id': 1, 'time': rt2}).run(c)
        self.assertEqual(res['inserted'], 1)

        expected_row1 = {'id': 0, 'time': dt1}
        expected_row2 = {'id': 1, 'time': dt2}

        groups = yield from r.table('times').group('time').coerce_to('array').run(c)
        self.assertEqual(groups, {dt1: [expected_row1],
                                  dt2: [expected_row2]})


class TestSuccessAtomFeed(TestWithConnection):
    @asyncio.coroutine
    def runTest(self):
        c = yield from r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)

        from rethinkdb import ql2_pb2 as p

        if 'success_atom_feed' in (yield from r.db('test').table_list().run(c)):
            yield from r.db('test').table_drop('success_atom_feed').run(c)
        yield from r.db('test').table_create('success_atom_feed').run(c)
        t1 = r.db('test').table('success_atom_feed')

        res = yield from t1.insert({'id': 0, 'a': 16}).run(c)
        self.assertEqual(res['inserted'], 1)
        res = yield from t1.insert({'id': 1, 'a': 31}).run(c)
        self.assertEqual(res['inserted'], 1)

        yield from t1.index_create('a', lambda x: x['a']).run(c)
        yield from t1.index_wait('a').run(c)

        changes = yield from t1.get(0).changes(include_initial=True).run(c)
        self.assertTrue(changes.error is None)
        self.assertEqual(len(changes.items), 1)

class TestCursor(TestWithConnection):
    @asyncio.coroutine
    def setUp(self):
        yield from TestWithConnection.setUp(self)
        self.conn = yield from r.connect(host=sharedServerHost,
                                    port=sharedServerDriverPort)

    @asyncio.coroutine
    def tearDown(self):
        yield from TestWithConnection.tearDown(self)
        yield from self.conn.close()
        self.conn = None

    @asyncio.coroutine
    def test_type(self):
        cursor = yield from r.range().run(self.conn)
        self.assertTrue(isinstance(cursor, r.Cursor))

    @asyncio.coroutine
    def test_cursor_after_connection_close(self):
        cursor = yield from r.range().run(self.conn)
        yield from self.conn.close()

        @asyncio.coroutine
        def read_cursor(cursor):
            while (yield from cursor.fetch_next()):
                yield from cursor.next()
                cursor.close()

        with self.assertRaisesRegexp(r.ReqlRuntimeError, "Connection is closed."):
             yield from read_cursor(cursor)

    @asyncio.coroutine
    def test_cursor_after_cursor_close(self):
        cursor = yield from r.range().run(self.conn)
        cursor.close()
        count = 0
        while (yield from cursor.fetch_next()):
            yield from cursor.next()
            count += 1
        self.assertNotEqual(count, 0, "Did not get any cursor results")

    @asyncio.coroutine
    def test_cursor_close_in_each(self):
        cursor = yield from r.range().run(self.conn)
        count = 0

        while (yield from cursor.fetch_next()):
            yield from cursor.next()
            count += 1
            if count == 2:
                cursor.close()

        self.assertTrue(count >= 2, "Did not get enough cursor results")

    @asyncio.coroutine
    def test_cursor_success(self):
        range_size = 10000
        cursor = yield from r.range().limit(range_size).run(self.conn)
        count = 0
        while (yield from cursor.fetch_next()):
            yield from cursor.next()
            count += 1
        self.assertEqual(count, range_size,
             "Expected %d results on the cursor, but got %d" % (range_size, count))

    @asyncio.coroutine
    def test_cursor_double_each(self):
        range_size = 10000
        cursor = yield from r.range().limit(range_size).run(self.conn)
        count = 0

        while (yield from cursor.fetch_next()):
            yield from cursor.next()
            count += 1
        self.assertEqual(count, range_size,
             "Expected %d results on the cursor, but got %d" % (range_size, count))

        while (yield from cursor.fetch_next()):
            yield from cursor.next()
            count += 1
        self.assertEqual(count, range_size,
             "Expected no results on the second iteration of the cursor, but got %d" % (count - range_size))

    # Used in wait tests
    num_cursors=3

    @asyncio.coroutine
    def do_wait_test(self, wait_time):
        cursors = [ ]
        cursor_counts = [ ]
        cursor_timeouts = [ ]
        for i in xrange(self.num_cursors):
            cur = yield from r.range().map(r.js("(function (row) {" +
                                                "end = new Date(new Date().getTime() + 2);" +
                                                "while (new Date() < end) { }" +
                                                "return row;" +
                                           "})")).run(self.conn, max_batch_rows=2)
            cursors.append(cur)
            cursor_counts.append(0)
            cursor_timeouts.append(0)

        @asyncio.coroutine
        def get_next(cursor_index):
            try:
                if wait_time is None: # Special case to use the default
                    yield from cursors[cursor_index].next()
                else:
                    yield from cursors[cursor_index].next(wait=wait_time)
                cursor_counts[cursor_index] += 1
            except r.ReqlTimeoutError:
                cursor_timeouts[cursor_index] += 1

        # We need to get ahead of pre-fetching for this to get the error we want
        while sum(cursor_counts) < 4000:
            for cursor_index in xrange(self.num_cursors):
                for read_count in xrange(random.randint(0, 10)):
                    yield from get_next(cursor_index)

        [cursor.close() for cursor in cursors]

        return (sum(cursor_counts), sum(cursor_timeouts))

    @asyncio.coroutine
    def test_false_wait(self):
        reads, timeouts = yield from self.do_wait_test(False)
        self.assertNotEqual(timeouts, 0,
            "Did not get timeouts using zero (false) wait.")

    @asyncio.coroutine
    def test_zero_wait(self):
        reads, timeouts = yield from self.do_wait_test(0)
        self.assertNotEqual(timeouts, 0,
            "Did not get timeouts using zero wait.")

    @asyncio.coroutine
    def test_short_wait(self):
        reads, timeouts = yield from self.do_wait_test(0.0001)
        self.assertNotEqual(timeouts, 0,
            "Did not get timeouts using short (100 microsecond) wait.")

    @asyncio.coroutine
    def test_long_wait(self):
        reads, timeouts = yield from self.do_wait_test(10)
        self.assertEqual(timeouts, 0,
            "Got timeouts using long (10 second) wait.")

    @asyncio.coroutine
    def test_infinite_wait(self):
        reads, timeouts = yield from self.do_wait_test(True)
        self.assertEqual(timeouts, 0,
            "Got timeouts using infinite wait.")

    @asyncio.coroutine
    def test_default_wait(self):
        reads, timeouts = yield from self.do_wait_test(None)
        self.assertEqual(timeouts, 0,
            "Got timeouts using default (infinite) wait.")

    # This test relies on the internals of the TornadoCursor implementation
    @asyncio.coroutine
    def test_rate_limit(self):
        # Get the first batch
        cursor = yield from r.range().run(self.conn)
        cursor_initial_size = len(cursor.items)

        # Wait for the second (pre-fetched) batch to arrive
        yield from cursor.new_response
        cursor_new_size = len(cursor.items)

        self.assertLess(cursor_initial_size, cursor_new_size)

        # Wait and observe that no third batch arrives
        with self.assertRaises(asyncio.TimeoutError):
            yield from asyncio.wait_for(asyncio.shield(cursor.new_response), 2)
        self.assertEqual(cursor_new_size, len(cursor.items))

    # Test that an error on a cursor (such as from closing the connection)
    # properly wakes up waiters immediately
    @asyncio.coroutine
    def test_cursor_error(self):
        cursor = yield from r.range() \
            .map(lambda row:
                r.branch(row <= 5000, # High enough for multiple batches
                         row,
                         r.js('while(true){ }'))) \
            .run(self.conn)

        @asyncio.coroutine
        def read_cursor(cursor, hanging):
            try:
                while True:
                    yield from cursor.next(wait=1)
            except r.ReqlTimeoutError:
                pass
            hanging.set_result(True)
            yield from cursor.next()

        @asyncio.coroutine
        def read_wrapper(cursor, done, hanging):
            try:
                with self.assertRaisesRegexp(r.ReqlRuntimeError,
                                             'Connection is closed.'):
                    yield from read_cursor(cursor, hanging)
                done.set_result(None)
            except Exception as ex:
                if not cursor_hanging.done():
                    cursor_hanging.set_exception(ex)
                done.set_exception(ex)

        cursor_hanging = asyncio.Future()
        done = asyncio.Future()
        asyncio.async(read_wrapper(cursor, done, cursor_hanging))

        # Wait for the cursor to hit the hang point before we close and cause an error
        yield from cursor_hanging
        yield from self.conn.close()
        yield from done

class TestChangefeeds(TestWithConnection):
    @asyncio.coroutine
    def setUp(self):
        yield from TestWithConnection.setUp(self)
        self.conn = yield from r.connect(host=sharedServerHost,
                                    port=sharedServerDriverPort)
        if 'a' in (yield from r.db('test').table_list().run(self.conn)):
            yield from r.db('test').table_drop('a').run(self.conn)
        yield from r.db('test').table_create('a').run(self.conn)
        if 'b' in (yield from r.db('test').table_list().run(self.conn)):
            yield from r.db('test').table_drop('b').run(self.conn)
        yield from r.db('test').table_create('b').run(self.conn)

    @asyncio.coroutine
    def tearDown(self):
        yield from r.db('test').table_drop('b').run(self.conn)
        yield from r.db('test').table_drop('a').run(self.conn)
        yield from TestWithConnection.tearDown(self)
        yield from self.conn.close()
        self.conn = None

    @asyncio.coroutine
    def table_a_even_writer(self):
        for i in range(10):
            yield from r.db('test').table("a").insert({"id": i * 2}).run(self.conn)

    @asyncio.coroutine
    def table_a_odd_writer(self):
        for i in range(10):
            yield from r.db('test').table("a").insert({"id": i * 2 + 1}).run(self.conn)

    @asyncio.coroutine
    def table_b_writer(self):
        for i in range(10):
            yield from r.db('test').table("b").insert({"id": i}).run(self.conn)

    @asyncio.coroutine
    def cfeed_noticer(self, table, ready, done, needed_values):
        feed = yield from r.db('test').table(table).changes(squash=False).run(self.conn)
        try:
            ready.set_result(None)
            while len(needed_values) != 0 and (yield from feed.fetch_next()):
                item = yield from feed.next()
                self.assertIsNone(item['old_val'])
                self.assertIn(item['new_val']['id'], needed_values)
                needed_values.remove(item['new_val']['id'])
            done.set_result(None)
        except Exception as ex:
            done.set_exception(ex)

    @asyncio.coroutine
    def test_multiple_changefeeds(self):
        loop = asyncio.get_event_loop()
        feeds_ready = { }
        feeds_done = { }
        needed_values = { 'a': set(range(20)), 'b': set(range(10)) }
        for n in ('a', 'b'):
            feeds_ready[n] = asyncio.Future()
            feeds_done[n] = asyncio.Future()
            asyncio.async(self.cfeed_noticer(n, feeds_ready[n], feeds_done[n],
                                             needed_values[n]))

        yield from asyncio.wait(feeds_ready.values())
        yield from asyncio.wait([self.table_a_even_writer(),
               self.table_a_odd_writer(),
               self.table_b_writer()])
        yield from asyncio.wait(feeds_done.values())
        self.assertTrue(all([len(x) == 0 for x in needed_values.values()]))

if __name__ == '__main__':
    print("Running asyncio connection tests")
    suite = unittest.TestSuite()
    loader = unittest.TestLoader()
    suite.addTest(loader.loadTestsFromTestCase(TestCursor))
    suite.addTest(loader.loadTestsFromTestCase(TestChangefeeds))
    suite.addTest(loader.loadTestsFromTestCase(TestNoConnection))
    suite.addTest(loader.loadTestsFromTestCase(TestConnection))
    suite.addTest(TestBatching())
    suite.addTest(TestGetIntersectingBatching())
    suite.addTest(TestGroupWithTimeKey())
    suite.addTest(TestSuccessAtomFeed())
    suite.addTest(loader.loadTestsFromTestCase(TestShutdown))

    res = unittest.TextTestRunner(stream=sys.stdout, verbosity=2).run(suite)

    serverClosedCleanly = True
    try:
        if sharedServer is not None:
            sharedServer.check_and_stop()
    except Exception as e:
        serverClosedCleanly = False
        sys.stderr.write('The server did not close cleanly after testing: %s'
                         % str(e))

    if not res.wasSuccessful() or not serverClosedCleanly:
        sys.exit(1)
