#!/usr/bin/env python
##
# Tests the driver API for making connections and exercizes the
# networking code
###

from __future__ import print_function

import datetime
import os
import random
import re
import sys
import tempfile
import time
import unittest
import socket

from twisted.internet import reactor, defer, threads
from twisted.internet.defer import inlineCallbacks, returnValue
from twisted.python.failure import Failure

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

r.set_loop_type("twisted")

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

def sleep(secs):
    d = defer.Deferred()
    reactor.callLater(secs, d.callback, None)
    return d


@inlineCallbacks
def checkSharedServer():
    if sharedServerDriverPort is not None:
        conn = yield r.connect(host=sharedServerHost,
                               port=sharedServerDriverPort)
        if 'test' not in (yield r.db_list().run(conn)):
            yield r.db_create('test').run(conn)


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


# == Test Base Classes

class TestCaseCompatible(unittest.TestCase):
    # can't use standard TestCase run here because async.
    def setUp(self):
        return defer.succeed(None)

    def tearDown(self):
        return defer.succeed(None)

    def run(self, result=None):
        if result is None:
            result = self.defaultTestResult()

        def treatFailure(failure, result):
            if failure is KeyboardInterrupt:
                raise failure
            else:
                result.addError(self, sys.exc_info())

        def markSuccess(_, result=None):
            result.addSuccess(self)
            return result

        result.startTest(self)
        testMethod = getattr(self, self._testMethodName)
        d = defer.maybeDeferred(self.setUp)
        d.addCallback(lambda x: defer.maybeDeferred(testMethod))
        d.addCallback(lambda x: defer.maybeDeferred(self.tearDown))
        d.addCallback(markSuccess, result=result)
        d.addErrback(treatFailure, result)
        d.addBoth(lambda x: result.stopTest(self))
        return d


class TestWithConnection(TestCaseCompatible):
    port = None
    server = None
    serverOutput = None
    ioloop = None

    @inlineCallbacks
    def setUp(self):
        global sharedServer, sharedServerOutput, sharedServerHost, \
            sharedServerDriverPort

        if sharedServer is not None:
            try:
                yield sharedServer.check()
            except Exception:
                # TODO: figure out how to blame the last test
                closeSharedServer()

        if sharedServerDriverPort is None:
            sharedServerOutput = tempfile.NamedTemporaryFile('w+')
            sharedServer = driver.Process(executable_path=rethinkdb_exe,
                                          console_output=sharedServerOutput,
                                          wait_until_ready=True)
            sharedServerHost = sharedServer.host
            sharedServerDriverPort = sharedServer.driver_port

        # - ensure we are ready

        yield checkSharedServer()

    @inlineCallbacks
    def tearDown(self):
        global sharedServer, sharedServerOutput, sharedServerHost, \
            sharedServerDriverPort

        if sharedServerDriverPort is not None:
            try:
                yield checkSharedServer()
            except Exception:
                closeSharedServer()
                raise  # TODO: figure out how to best give the server log
        else:
            returnValue(None)

# == Test Classes


class TestNoConnection(TestCaseCompatible):
    # No servers started yet so this should fail
    @inlineCallbacks
    def test_connect(self):
        if not use_default_port:
            returnValue(None)  # skipTest will not raise in this environment
        with self.assertRaisesRegexp(r.ReqlDriverError,
                                           "Could not connect to localhost:%d."
                                           % DEFAULT_DRIVER_PORT):
            yield r.connect()

    @inlineCallbacks
    def test_connect_port(self):
        port = utils.get_avalible_port()
        with self.assertRaisesRegexp(r.ReqlDriverError,
                                           "Could not connect to localhost:%d."
                                           % port):
            yield r.connect(port=port)

    @inlineCallbacks
    def test_connect_host(self):
        if not use_default_port:
            returnValue(None)  # skipTest will not raise in this environment
        with self.assertRaisesRegexp(r.ReqlDriverError,
                                           "Could not connect to 0.0.0.0:%d."
                                           % DEFAULT_DRIVER_PORT):
            yield r.connect(host="0.0.0.0")

    @inlineCallbacks
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
                yield r.connect(host=host, port=port, timeout=2)
        finally:
            useSocket.close()

    @inlineCallbacks
    def test_empty_run(self):
        # Test the error message when we pass nothing to run and
        # didn't call `repl`
        with self.assertRaisesRegexp(r.ReqlDriverError,
                                "RqlQuery.run must be given"
                                " a connection to run on."):
             yield r.expr(1).run()

    @inlineCallbacks
    def test_auth_key(self):
        # Test that everything still doesn't work even with an auth key
        if not use_default_port:
            returnValue(None)  # skipTest will not raise in this environment
        with self.assertRaisesRegexp(r.ReqlDriverError,
                                           'Could not connect to 0.0.0.0:%d."'
                                           % DEFAULT_DRIVER_PORT):
            yield r.connect(host="0.0.0.0", port=DEFAULT_DRIVER_PORT,
                                 auth_key="hunter2")


class TestConnection(TestWithConnection):
    @inlineCallbacks
    def test_connect_close_reconnect(self):
        c = yield r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        yield r.expr(1).run(c)
        yield c.close()
        yield c.close()
        yield c.reconnect()
        yield r.expr(1).run(c)

    @inlineCallbacks
    def test_connect_close_expr(self):
        c = yield r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        yield r.expr(1).run(c)
        yield c.close()
        with self.assertRaisesRegexp(r.ReqlDriverError, "Connection is closed."):
            yield r.expr(1).run(c)

    @inlineCallbacks
    def test_noreply_wait_waits(self):
        c = yield r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        t = time.time()
        yield r.js('while(true);', timeout=0.5).run(c, noreply=True)
        yield c.noreply_wait()
        duration = time.time() - t
        self.assertGreaterEqual(duration, 0.5)

    @inlineCallbacks
    def test_close_waits_by_default(self):
        c = yield r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        t = time.time()
        yield r.js('while(true);', timeout=0.5).run(c, noreply=True)
        yield c.close()
        duration = time.time() - t
        self.assertGreaterEqual(duration, 0.5)

    @inlineCallbacks
    def test_reconnect_waits_by_default(self):
        c = yield r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        t = time.time()
        yield r.js('while(true);', timeout=0.5).run(c, noreply=True)
        yield c.reconnect()
        duration = time.time() - t
        self.assertGreaterEqual(duration, 0.5)

    @inlineCallbacks
    def test_close_does_not_wait_if_requested(self):
        c = yield r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        t = time.time()
        yield r.js('while(true);', timeout=0.5).run(c, noreply=True)
        yield c.close(noreply_wait=False)
        duration = time.time() - t
        self.assertLess(duration, 0.5)

    @inlineCallbacks
    def test_reconnect_does_not_wait_if_requested(self):
        c = yield r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        t = time.time()
        yield r.js('while(true);', timeout=0.5).run(c, noreply=True)
        yield c.reconnect(noreply_wait=False)
        duration = time.time() - t
        self.assertLess(duration, 0.5)

    @inlineCallbacks
    def test_db(self):
        c = yield r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)

        if 't1' in (yield r.db('test').table_list().run(c)):
            yield r.db('test').table_drop('t1').run(c)
        yield r.db('test').table_create('t1').run(c)

        if 'db2' in (yield r.db_list().run(c)):
            yield r.db_drop('db2').run(c)
        yield r.db_create('db2').run(c)

        if 't2' in (yield r.db('db2').table_list().run(c)):
            yield r.db('db2').table_drop('t2').run(c)
        yield r.db('db2').table_create('t2').run(c)

        # Default db should be 'test' so this will work
        yield r.table('t1').run(c)

        # Use a new database
        c.use('db2')
        yield r.table('t2').run(c)
        with self.assertRaisesRegexp(r.ReqlRuntimeError,
                                           "Table `db2.t1` does not exist."):
            yield r.table('t1').run(c)

        c.use('test')
        yield r.table('t1').run(c)
        with self.assertRaisesRegexp(r.ReqlRuntimeError,
                                           "Table `test.t2` does not exist."):
            yield r.table('t2').run(c)

        yield c.close()

        # Test setting the db in connect
        c = yield r.connect(db='db2', host=sharedServerHost,
                            port=sharedServerDriverPort)
        yield r.table('t2').run(c)

        with self.assertRaisesRegexp(r.ReqlRuntimeError,
                                           "Table `db2.t1` does not exist."):
            yield r.table('t1').run(c)

        yield c.close()

        # Test setting the db as a `run` option
        c = yield r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        yield r.table('t2').run(c, db='db2')

    @inlineCallbacks
    def test_repl(self):
        # Calling .repl() should set this connection as global state
        # to be used when `run` is not otherwise passed a connection.
        c = (yield r.connect(host=sharedServerHost,
                             port=sharedServerDriverPort)).repl()

        yield r.expr(1).run()

        c.repl()                # is idempotent

        yield r.expr(1).run()

        yield c.close()

        with self.assertRaisesRegexp(r.ReqlDriverError, "Connection is closed."):
            yield r.expr(1).run()

    @inlineCallbacks
    def test_port_conversion(self):
        c = yield r.connect(host=sharedServerHost,
                            port=str(sharedServerDriverPort))
        yield r.expr(1).run(c)
        yield c.close()

        with self.assertRaisesRegexp(r.ReqlDriverError,
                               "Could not convert port abc to an integer."):
            yield r.connect(port='abc', host=sharedServerHost)


class TestShutdown(TestWithConnection):
    @inlineCallbacks
    def setUp(self):
        if sharedServer is None:
            # we need to be able to kill the server, so can't use one
            # from outside
            yield closeSharedServer()
        yield super(TestShutdown, self).setUp()

    @inlineCallbacks
    def test_shutdown(self):
        c = yield r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        yield r.expr(1).run(c)

        closeSharedServer()
        yield sleep(0.02)

        with self.assertRaisesRegexp(r.ReqlDriverError,
                               "Connection is closed."):
            yield r.expr(1).run(c)


# Another non-connection connection test. It's to test that get_intersecting()
# batching works properly.
class TestGetIntersectingBatching(TestWithConnection):
    @inlineCallbacks
    def runTest(self):
        c = yield r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)

        if 't1' in (yield r.db('test').table_list().run(c)):
            yield r.db('test').table_drop('t1').run(c)
        yield r.db('test').table_create('t1').run(c)
        t1 = r.db('test').table('t1')

        yield t1.index_create('geo', geo=True).run(c)
        yield t1.index_wait('geo').run(c)

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
        yield t1.insert(points).run(c)
        yield t1.insert(polygons).run(c)

        # Check that the results are actually lazy at least some of
        # the time While the test is randomized, chances are extremely
        # high to get a lazy result at least once.
        seen_lazy = False

        for i in xrange(0, get_tries):
            query_circle = r.circle([random.uniform(-180.0, 180.0),
                                     random.uniform(-90.0, 90.0)], 8000000)
            reference = yield t1.filter(r.row['geo'].intersects(query_circle))\
                                .coerce_to("ARRAY").run(c)
            cursor = yield t1.get_intersecting(query_circle, index='geo')\
                             .run(c, max_batch_rows=batch_size)
            if cursor.error is None:
                seen_lazy = True

            while len(reference) > 0:
                row = yield cursor.next()
                self.assertEqual(reference.count(row), 1)
                reference.remove(row)
            with self.assertRaises(r.ReqlCursorEmpty):
                yield cursor.next()

        self.assertTrue(seen_lazy)

        yield r.db('test').table_drop('t1').run(c)


class TestBatching(TestWithConnection):
    @inlineCallbacks
    def runTest(self):
        c = yield r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)

        # Test the cursor API when there is exactly mod batch size
        # elements in the result stream
        if 't1' in (yield r.db('test').table_list().run(c)):
            yield r.db('test').table_drop('t1').run(c)
        yield r.db('test').table_create('t1').run(c)
        t1 = r.table('t1')

        batch_size = 3
        count = 500

        ids = set(xrange(0, count))

        yield t1.insert([{'id': i} for i in ids]).run(c)
        cursor = yield t1.run(c, max_batch_rows=batch_size)

        for i in xrange(count - 1):
            row = yield cursor.next()
            self.assertTrue(row['id'] in ids)
            ids.remove(row['id'])


        self.assertEqual((yield cursor.next())['id'], ids.pop())
        with self.assertRaises(r.ReqlCursorEmpty):
            yield cursor.next()
        yield r.db('test').table_drop('t1').run(c)


class TestGroupWithTimeKey(TestWithConnection):
    @inlineCallbacks
    def runTest(self):
        c = yield r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)

        if 'times' in (yield r.db('test').table_list().run(c)):
            yield r.db('test').table_drop('times').run(c)
        yield r.db('test').table_create('times').run(c)

        time1 = 1375115782.24
        rt1 = r.epoch_time(time1).in_timezone('+00:00')
        dt1 = datetime.datetime.fromtimestamp(time1, r.ast.RqlTzinfo('+00:00'))
        time2 = 1375147296.68
        rt2 = r.epoch_time(time2).in_timezone('+00:00')
        dt2 = datetime.datetime.fromtimestamp(time2, r.ast.RqlTzinfo('+00:00'))

        res = yield r.table('times').insert({'id': 0, 'time': rt1}).run(c)
        self.assertEqual(res['inserted'], 1)
        res = yield r.table('times').insert({'id': 1, 'time': rt2}).run(c)
        self.assertEqual(res['inserted'], 1)

        expected_row1 = {'id': 0, 'time': dt1}
        expected_row2 = {'id': 1, 'time': dt2}

        groups = yield r.table('times').group('time').coerce_to('array').run(c)
        self.assertEqual(groups, {dt1: [expected_row1],
                                  dt2: [expected_row2]})


class TestSuccessAtomFeed(TestWithConnection):
    @inlineCallbacks
    def runTest(self):
        c = yield r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)

        from rethinkdb import ql2_pb2 as p

        if 'success_atom_feed' in (yield r.db('test').table_list().run(c)):
            yield r.db('test').table_drop('success_atom_feed').run(c)
        yield r.db('test').table_create('success_atom_feed').run(c)
        t1 = r.db('test').table('success_atom_feed')

        res = yield t1.insert({'id': 0, 'a': 16}).run(c)
        self.assertEqual(res['inserted'], 1)
        res = yield t1.insert({'id': 1, 'a': 31}).run(c)
        self.assertEqual(res['inserted'], 1)

        yield t1.index_create('a', lambda x: x['a']).run(c)
        yield t1.index_wait('a').run(c)

        changes = yield t1.get(0).changes(include_initial=True).run(c)
        self.assertTrue(changes.error is None)
        self.assertEqual(len(changes.items), 1)

class TestCursor(TestWithConnection):
    @inlineCallbacks
    def setUp(self):
        yield TestWithConnection.setUp(self)
        self.conn = yield r.connect(host=sharedServerHost,
                                    port=sharedServerDriverPort)

    @inlineCallbacks
    def tearDown(self):
        yield TestWithConnection.tearDown(self)
        yield self.conn.close()
        self.conn = None

    @inlineCallbacks
    def test_type(self):
        cursor = yield r.range().run(self.conn)
        self.assertTrue(isinstance(cursor, r.Cursor))

    @inlineCallbacks
    def test_cursor_after_connection_close(self):
        cursor = yield r.range().run(self.conn)
        yield self.conn.close()

        @inlineCallbacks
        def read_cursor(cursor):
            while (yield cursor.fetch_next()):
                item = yield cursor.next()
                cursor.close()

        with self.assertRaisesRegexp(r.ReqlRuntimeError, "Connection is closed."):
             yield read_cursor(cursor)

    @inlineCallbacks
    def test_cursor_after_cursor_close(self):
        cursor = yield r.range().run(self.conn)
        cursor.close()
        count = 0
        while (yield cursor.fetch_next()):
            item = yield cursor.next()
            count += 1

        self.assertNotEqual(count, 0, "Did not get any cursor results")

    @inlineCallbacks
    def test_cursor_close_in_each(self):
        cursor = yield r.range().run(self.conn)
        count = 0

        while (yield cursor.fetch_next()):
            item = yield cursor.next()
            count += 1
            if count == 2:
                cursor.close()

        self.assertTrue(count >= 2, "Did not get enough cursor results")

    @inlineCallbacks
    def test_cursor_success(self):
        range_size = 10000
        cursor = yield r.range().limit(range_size).run(self.conn)
        count = 0
        while (yield cursor.fetch_next()):
            item = yield cursor.next()
            count += 1
        self.assertEqual(count, range_size,
             "Expected %d results on the cursor, but got %d" % (range_size, count))

    @inlineCallbacks
    def test_cursor_double_each(self):
        range_size = 10000
        cursor = yield r.range().limit(range_size).run(self.conn)
        count = 0

        while (yield cursor.fetch_next()):
            item = yield cursor.next()
            count += 1
        self.assertEqual(count, range_size,
             "Expected %d results on the cursor, but got %d" % (range_size, count))

        while (yield cursor.fetch_next()):
            item = yield cursor.next()
            count += 1
        self.assertEqual(count, range_size,
             "Expected no results on the second iteration of the cursor, but got %d" % (count - range_size))

    # Used in wait tests
    num_cursors=3

    @inlineCallbacks
    def do_wait_test(self, wait_time):
        cursors = [ ]
        cursor_counts = [ ]
        cursor_timeouts = [ ]
        for i in xrange(self.num_cursors):
            cur = yield r.range().map(r.js("(function (row) {" +
                                                "end = new Date(new Date().getTime() + 2);" +
                                                "while (new Date() < end) { }" +
                                                "return row;" +
                                           "})")).run(self.conn, max_batch_rows=2)
            cursors.append(cur)
            cursor_counts.append(0)
            cursor_timeouts.append(0)

        @inlineCallbacks
        def get_next(cursor_index):
            try:
                if wait_time is None: # Special case to use the default
                    yield cursors[cursor_index].next()
                else:
                    yield cursors[cursor_index].next(wait=wait_time)
                cursor_counts[cursor_index] += 1
            except r.ReqlTimeoutError:
                cursor_timeouts[cursor_index] += 1

        # We need to get ahead of pre-fetching for this to get the error we want
        while sum(cursor_counts) < 4000:
            for cursor_index in xrange(self.num_cursors):
                for read_count in xrange(random.randint(0, 10)):
                    yield get_next(cursor_index)

        [cursor.close() for cursor in cursors]

        returnValue((sum(cursor_counts), sum(cursor_timeouts)))

    @inlineCallbacks
    def test_false_wait(self):
        reads, timeouts = yield self.do_wait_test(False)
        self.assertNotEqual(timeouts, 0,
            "Did not get timeouts using zero (false) wait.")

    @inlineCallbacks
    def test_zero_wait(self):
        reads, timeouts = yield self.do_wait_test(0)
        self.assertNotEqual(timeouts, 0,
            "Did not get timeouts using zero wait.")

    @inlineCallbacks
    def test_short_wait(self):
        reads, timeouts = yield self.do_wait_test(0.0001)
        self.assertNotEqual(timeouts, 0,
            "Did not get timeouts using short (100 microsecond) wait.")

    @inlineCallbacks
    def test_long_wait(self):
        reads, timeouts = yield self.do_wait_test(10)
        self.assertEqual(timeouts, 0,
            "Got timeouts using long (10 second) wait.")

    @inlineCallbacks
    def test_infinite_wait(self):
        reads, timeouts = yield self.do_wait_test(True)
        self.assertEqual(timeouts, 0,
            "Got timeouts using infinite wait.")

    @inlineCallbacks
    def test_default_wait(self):
        reads, timeouts = yield self.do_wait_test(None)
        self.assertEqual(timeouts, 0,
            "Got timeouts using default (infinite) wait.")

    # This test relies on the internals of the TwistedCursor implementation
    @inlineCallbacks
    def test_rate_limit(self):
        # Get the first batch
        cursor = yield r.range().run(self.conn)
        cursor_initial_size = len(cursor.items)

        # Wait for the second (pre-fetched) batch to arrive
        new_response = defer.Deferred()
        reactor.callLater(2, lambda: new_response.cancel())
        cursor.waiting.append(new_response)
        yield new_response

        cursor_new_size = len(cursor.items)

        self.assertLess(cursor_initial_size, cursor_new_size)


        # Wait and observe that no third batch arrives
        with self.assertRaises(defer.CancelledError):
            new_response = defer.Deferred()
            reactor.callLater(2, lambda: new_response.cancel())
            cursor.waiting.append(new_response)
            yield new_response

        self.assertEqual(cursor_new_size, len(cursor.items))

    # Test that an error on a cursor (such as from closing the connection)
    # properly wakes up waiters immediately
    @inlineCallbacks
    def test_cursor_error(self):
        cursor = yield r.range() \
            .map(lambda row:
                r.branch(row <= 5000, # High enough for multiple batches
                         row,
                         r.js('while(true){ }'))) \
            .run(self.conn)

        @inlineCallbacks
        def read_cursor(cursor, hanging):
            try:
                while True:
                    yield cursor.next(wait=1)
            except r.ReqlTimeoutError:
                pass
            hanging.callback(None)
            yield cursor.next()

        @inlineCallbacks
        def read_wrapper(cursor, hanging):
            try:
                with self.assertRaisesRegexp(r.ReqlRuntimeError,
                                             'Connection is closed.'):
                    yield read_cursor(cursor, hanging)
            except Exception as ex:
                if not cursor_hanging.called:
                    cursor_hanging.errback(ex)
                raise ex

        cursor_hanging = defer.Deferred()
        done = read_wrapper(cursor, cursor_hanging)

        # Wait for the cursor to hit the hang point before we close and cause an error
        yield cursor_hanging
        yield self.conn.close()
        yield done

class TestChangefeeds(TestWithConnection):
    @inlineCallbacks
    def setUp(self):
        yield TestWithConnection.setUp(self)
        self.conn = yield r.connect(host=sharedServerHost,
                                    port=sharedServerDriverPort)
        if 'a' in (yield r.db('test').table_list().run(self.conn)):
            yield r.db('test').table_drop('a').run(self.conn)
        yield r.db('test').table_create('a').run(self.conn)
        if 'b' in (yield r.db('test').table_list().run(self.conn)):
            yield r.db('test').table_drop('b').run(self.conn)
        yield r.db('test').table_create('b').run(self.conn)

    @inlineCallbacks
    def tearDown(self):
        yield r.db('test').table_drop('b').run(self.conn)
        yield r.db('test').table_drop('a').run(self.conn)
        yield TestWithConnection.tearDown(self)
        yield self.conn.close()
        self.conn = None

    @inlineCallbacks
    def table_a_even_writer(self):
        for i in range(10):
            yield r.db('test').table("a").insert({"id": i * 2}).run(self.conn)

    @inlineCallbacks
    def table_a_odd_writer(self):
        for i in range(10):
            yield r.db('test').table("a").insert({"id": i * 2 + 1}).run(self.conn)

    @inlineCallbacks
    def table_b_writer(self):
        for i in range(10):
            yield r.db('test').table("b").insert({"id": i}).run(self.conn)

    @inlineCallbacks
    def cfeed_noticer(self, table, ready, needed_values):
        feed = yield r.db('test').table(table).changes(squash=False).run(self.conn)
        ready.callback(None)
        while len(needed_values) > 0:
            item = yield feed.next()
            self.assertIsNone(item['old_val'])
            self.assertIn(item['new_val']['id'], needed_values)
            needed_values.remove(item['new_val']['id'])

    @inlineCallbacks
    def test_multiple_changefeeds(self):
        feeds_ready = {}
        feeds_done = {}
        needed_values = { 'a': set(range(20)), 'b': set(range(10)) }
        for n in ('a', 'b'):
            feeds_ready[n] = defer.Deferred()
            feeds_done[n] = self.cfeed_noticer(n, feeds_ready[n], needed_values[n])

        yield defer.DeferredList(feeds_ready.values())
        yield defer.DeferredList([self.table_a_even_writer(),
               self.table_a_odd_writer(),
               self.table_b_writer()])
        yield defer.DeferredList(feeds_done.values())
        self.assertTrue(all([len(x) == 0 for x in needed_values.values()]))

class AsyncSuite(unittest.TestSuite):

    def run(self, result):
        def _runTest(res, test, result):
            return test(result)

        d = None
        for test in self:
            if d is None:
                d = test(result)
            else:
                d = d.addCallback(_runTest, test, result)
        d.addCallback(lambda x: defer.succeed(result))
        return d

class AsyncTestRunner(unittest.TextTestRunner):

    def run(self, test):
        result = self._makeResult()
        startTime = time.time()
        def finishTest(result):
            stopTime = time.time()
            timeTaken = stopTime - startTime
            result.printErrors()
            self.stream.writeln(result.separator2)
            run = result.testsRun
            self.stream.writeln("Ran %d test%s in %.3fs" %
                                (run, run != 1 and "s" or "", timeTaken))
            self.stream.writeln()
            if not result.wasSuccessful():
                self.stream.write("FAILED (")
                failed, errored = map(len, (result.failures, result.errors))
                if failed:
                    self.stream.write("failures=%d" % failed)
                if errored:
                    if failed: self.stream.write(", ")
                    self.stream.write("errors=%d" % errored)
                self.stream.writeln(")")
            else:
                self.stream.writeln("OK")
            return defer.succeed(result)
        return test(result).addCallback(finishTest)

class AsyncTestLoader(unittest.TestLoader):

    suiteClass = AsyncSuite


if __name__ == '__main__':
    print("Running Twisted connection tests")
    defer.setDebugging(True)
    suite = AsyncSuite()
    loader = AsyncTestLoader()
    suite.addTest(loader.loadTestsFromTestCase(TestCursor))
    suite.addTest(loader.loadTestsFromTestCase(TestChangefeeds))
    suite.addTest(loader.loadTestsFromTestCase(TestNoConnection))
    suite.addTest(loader.loadTestsFromTestCase(TestConnection))
    suite.addTest(TestBatching())
    suite.addTest(TestGetIntersectingBatching())
    suite.addTest(TestGroupWithTimeKey())
    suite.addTest(TestSuccessAtomFeed())
    suite.addTest(loader.loadTestsFromTestCase(TestShutdown))

    def finishTests(res):
        serverClosedCleanly = True
        try:
            if sharedServer is not None:
                sharedServer.check_and_stop()
        except Exception as e:
            serverClosedCleanly = False
            sys.stderr.write('The server did not close cleanly after testing: %s'
                            % str(e))

        reactor.stop()
        if not res.wasSuccessful() or not serverClosedCleanly:
            sys.exit(1)

    AsyncTestRunner(stream=sys.stdout, verbosity=2).run(suite).\
        addCallback(finishTests)
    reactor.run()

