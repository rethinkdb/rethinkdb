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


async def checkSharedServer():
    if sharedServerDriverPort is not None:
        conn = await r.connect(host=sharedServerHost,
                               port=sharedServerDriverPort)
        if 'test' not in (await r.db_list().run(conn)):
            await r.db_create('test').run(conn)


async def closeSharedServer():
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

    async def setUp(self):
        return None

    async def tearDown(self):
        return None

    async def arun(self, result=None):
        if result is None:
            result = self.defaultTestResult()
        result.startTest(self)
        testMethod = getattr(self, self._testMethodName)

        try:
            try:
                await self.setUp()
            except KeyboardInterrupt:
                raise
            except:
                result.addError(self, sys.exc_info())
                return

            ok = False
            try:
                await testMethod()
                ok = True
            except self.failureException:
                result.addFailure(self, sys.exc_info())
            except KeyboardInterrupt:
                raise
            except:
                result.addError(self, sys.exc_info())

            try:
                await self.tearDown()
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

    async def setUp(self):
        global sharedServer, sharedServerOutput, sharedServerHost, \
            sharedServerDriverPort

        if sharedServer is not None:
            try:
                await sharedServer.check()
            except Exception:
                # ToDo: figure out how to blame the last test
                await closeSharedServer()

        if sharedServerDriverPort is None:
            sharedServerOutput = tempfile.NamedTemporaryFile('w+')
            sharedServer = driver.Process(executable_path=rethinkdb_exe,
                                          console_output=sharedServerOutput,
                                          wait_until_ready=True)
            sharedServerHost = sharedServer.host
            sharedServerDriverPort = sharedServer.driver_port

        # - insure we are ready

        await checkSharedServer()

    async def tearDown(self):
        global sharedServer, sharedServerOutput, sharedServerHost, \
            sharedServerDriverPort

        if sharedServerDriverPort is not None:
            try:
                await checkSharedServer()
            except Exception:
                await closeSharedServer()
                raise  # ToDo: figure out how to best give the server log

# == Test Classes


class TestNoConnection(TestCaseCompatible):
    # No servers started yet so this should fail
    async def test_connect(self):
        if not use_default_port:
            return None  # skipTest will not raise in this environment
        with self.assertRaisesRegexp(r.ReqlDriverError,
                                           "Could not connect to localhost:%d."
                                           % DEFAULT_DRIVER_PORT):
            await r.connect()

    async def test_connect_port(self):
        port = utils.get_avalible_port()
        with self.assertRaisesRegexp(r.ReqlDriverError,
                                           "Could not connect to localhost:%d."
                                           % port):
            await r.connect(port=port)

    async def test_connect_host(self):
        if not use_default_port:
            return None  # skipTest will not raise in this environment
        with self.assertRaisesRegexp(r.ReqlDriverError,
                                           "Could not connect to 0.0.0.0:%d."
                                           % DEFAULT_DRIVER_PORT):
            await r.connect(host="0.0.0.0")

    async def test_connect_timeout(self):
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
                await r.connect(host=host, port=port, timeout=2)
        finally:
            useSocket.close()

    async def test_empty_run(self):
        # Test the error message when we pass nothing to run and
        # didn't call `repl`
        with self.assertRaisesRegexp(r.ReqlDriverError,
                                "RqlQuery.run must be given"
                                " a connection to run on."):
             await r.expr(1).run()

    async def test_auth_key(self):
        # Test that everything still doesn't work even with an auth key
        if not use_default_port:
            return None  # skipTest will not raise in this environment
        with self.assertRaisesRegexp(r.ReqlDriverError,
                                           'Could not connect to 0.0.0.0:%d."'
                                           % DEFAULT_DRIVER_PORT):
            await r.connect(host="0.0.0.0", port=DEFAULT_DRIVER_PORT,
                                 auth_key="hunter2")


class TestConnection(TestWithConnection):
    async def test_connect_close_reconnect(self):
        c = await r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        await r.expr(1).run(c)
        await c.close()
        await c.close()
        await c.reconnect()
        await r.expr(1).run(c)

    async def test_async_with_syntax(self):
        async with r.connect(host=sharedServerHost, port=sharedServerDriverPort) as c:
            await r.expr(1).run(c)

    async def test_connect_close_expr(self):
        c = await r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        await r.expr(1).run(c)
        await c.close()
        async with self.assertRaisesRegexp(r.ReqlDriverError, "Connection is closed."):
            await r.expr(1).run(c)

    async def test_noreply_wait_waits(self):
        c = await r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        t = time.time()
        await r.js('while(true);', timeout=0.5).run(c, noreply=True)
        await c.noreply_wait()
        duration = time.time() - t
        self.assertGreaterEqual(duration, 0.5)

    async def test_close_waits_by_default(self):
        c = await r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        t = time.time()
        await r.js('while(true);', timeout=0.5).run(c, noreply=True)
        await c.close()
        duration = time.time() - t
        self.assertGreaterEqual(duration, 0.5)

    async def test_reconnect_waits_by_default(self):
        c = await r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        t = time.time()
        await r.js('while(true);', timeout=0.5).run(c, noreply=True)
        await c.reconnect()
        duration = time.time() - t
        self.assertGreaterEqual(duration, 0.5)

    async def test_close_does_not_wait_if_requested(self):
        c = await r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        t = time.time()
        await r.js('while(true);', timeout=0.5).run(c, noreply=True)
        await c.close(noreply_wait=False)
        duration = time.time() - t
        self.assertLess(duration, 0.5)

    async def test_reconnect_does_not_wait_if_requested(self):
        c = await r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        t = time.time()
        await r.js('while(true);', timeout=0.5).run(c, noreply=True)
        await c.reconnect(noreply_wait=False)
        duration = time.time() - t
        self.assertLess(duration, 0.5)

    async def test_db(self):
        c = await r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)

        if 't1' in (await r.db('test').table_list().run(c)):
            await r.db('test').table_drop('t1').run(c)
        await r.db('test').table_create('t1').run(c)

        if 'db2' in (await r.db_list().run(c)):
            await r.db_drop('db2').run(c)
        await r.db_create('db2').run(c)

        if 't2' in (await r.db('db2').table_list().run(c)):
            await r.db('db2').table_drop('t2').run(c)
        await r.db('db2').table_create('t2').run(c)

        # Default db should be 'test' so this will work
        await r.table('t1').run(c)

        # Use a new database
        c.use('db2')
        await r.table('t2').run(c)
        with self.assertRaisesRegexp(r.ReqlRuntimeError,
                                           "Table `db2.t1` does not exist."):
            await r.table('t1').run(c)

        c.use('test')
        await r.table('t1').run(c)
        with self.assertRaisesRegexp(r.ReqlRuntimeError,
                                           "Table `test.t2` does not exist."):
            await r.table('t2').run(c)

        await c.close()

        # Test setting the db in connect
        c = await r.connect(db='db2', host=sharedServerHost,
                            port=sharedServerDriverPort)
        await r.table('t2').run(c)

        with self.assertRaisesRegexp(r.ReqlRuntimeError,
                                           "Table `db2.t1` does not exist."):
            await r.table('t1').run(c)

        await c.close()

        # Test setting the db as a `run` option
        c = await r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        await r.table('t2').run(c, db='db2')

    async def test_outdated_read(self):
        c = await r.connect(host=sharedServerHost,
                                 port=sharedServerDriverPort)

        if 't1' in (await r.db('test').table_list().run(c)):
            await r.db('test').table_drop('t1').run(c)
        await r.db('test').table_create('t1').run(c)

        # Use outdated is an option that can be passed to db.table or `run`
        # We're just testing here if the server actually accepts the option.

        await r.table('t1', read_mode='outdated').run(c)
        await r.table('t1').run(c, read_mode='outdated')

    async def test_repl(self):
        # Calling .repl() should set this connection as global state
        # to be used when `run` is not otherwise passed a connection.
        c = (await r.connect(host=sharedServerHost,
                             port=sharedServerDriverPort)).repl()

        await r.expr(1).run()

        c.repl()                # is idempotent

        await r.expr(1).run()

        await c.close()

        with self.assertRaisesRegexp(r.ReqlDriverError, "Connection is closed."):
            await r.expr(1).run()

    async def test_port_conversion(self):
        c = await r.connect(host=sharedServerHost,
                            port=str(sharedServerDriverPort))
        await r.expr(1).run(c)
        await c.close()

        with self.assertRaisesRegexp(r.ReqlDriverError,
                               "Could not convert port abc to an integer."):
            await r.connect(port='abc', host=sharedServerHost)


class TestShutdown(TestWithConnection):
    async def setUp(self):
        if sharedServer is None:
            # we need to be able to kill the server, so can't use one
            # from outside
            await closeSharedServer()
        await super(TestShutdown, self).setUp()

    async def test_shutdown(self):
        c = await r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)
        await r.expr(1).run(c)

        await closeSharedServer()
        await asyncio.sleep(0.2)

        with self.assertRaisesRegexp(r.ReqlDriverError,
                               "Connection is closed."):
            await r.expr(1).run(c)


# Another non-connection connection test. It's to test that get_intersecting()
# batching works properly.
class TestGetIntersectingBatching(TestWithConnection):
    async def runTest(self):
        c = await r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)

        if 't1' in (await r.db('test').table_list().run(c)):
            await r.db('test').table_drop('t1').run(c)
        await r.db('test').table_create('t1').run(c)
        t1 = r.db('test').table('t1')

        await t1.index_create('geo', geo=True).run(c)
        await t1.index_wait('geo').run(c)

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
        await t1.insert(points).run(c)
        await t1.insert(polygons).run(c)

        # Check that the results are actually lazy at least some of
        # the time While the test is randomized, chances are extremely
        # high to get a lazy result at least once.
        seen_lazy = False

        for i in xrange(0, get_tries):
            query_circle = r.circle([random.uniform(-180.0, 180.0),
                                     random.uniform(-90.0, 90.0)], 8000000)
            reference = await t1.filter(r.row['geo'].intersects(query_circle))\
                                .coerce_to("ARRAY").run(c)
            cursor = await t1.get_intersecting(query_circle, index='geo')\
                             .run(c, max_batch_rows=batch_size)
            if cursor.error is None:
                seen_lazy = True

            while len(reference) > 0:
                row = await cursor.next()
                self.assertEqual(reference.count(row), 1)
                reference.remove(row)
            with self.assertRaises(r.ReqlCursorEmpty):
                await cursor.next()

        self.assertTrue(seen_lazy)

        await r.db('test').table_drop('t1').run(c)


class TestBatching(TestWithConnection):
    async def runTest(self):
        c = await r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)

        # Test the cursor API when there is exactly mod batch size
        # elements in the result stream
        if 't1' in (await r.db('test').table_list().run(c)):
            await r.db('test').table_drop('t1').run(c)
        await r.db('test').table_create('t1').run(c)
        t1 = r.table('t1')

        batch_size = 3
        count = 500

        ids = set(xrange(0, count))

        await t1.insert([{'id': i} for i in ids]).run(c)
        cursor = await t1.run(c, max_batch_rows=batch_size)

        for i in xrange(0, count - 1):
            row = await cursor.next()
            self.assertTrue(row['id'] in ids)
            ids.remove(row['id'])

        self.assertEqual((await cursor.next())['id'], ids.pop())
        with self.assertRaises(r.ReqlCursorEmpty):
            await cursor.next()
        await r.db('test').table_drop('t1').run(c)


class TestGroupWithTimeKey(TestWithConnection):
    async def runTest(self):
        c = await r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)

        if 'times' in (await r.db('test').table_list().run(c)):
            await r.db('test').table_drop('times').run(c)
        await r.db('test').table_create('times').run(c)

        time1 = 1375115782.24
        rt1 = r.epoch_time(time1).in_timezone('+00:00')
        dt1 = datetime.datetime.fromtimestamp(time1, r.ast.RqlTzinfo('+00:00'))
        time2 = 1375147296.68
        rt2 = r.epoch_time(time2).in_timezone('+00:00')
        dt2 = datetime.datetime.fromtimestamp(time2, r.ast.RqlTzinfo('+00:00'))

        res = await r.table('times').insert({'id': 0, 'time': rt1}).run(c)
        self.assertEqual(res['inserted'], 1)
        res = await r.table('times').insert({'id': 1, 'time': rt2}).run(c)
        self.assertEqual(res['inserted'], 1)

        expected_row1 = {'id': 0, 'time': dt1}
        expected_row2 = {'id': 1, 'time': dt2}

        groups = await r.table('times').group('time').coerce_to('array').run(c)
        self.assertEqual(groups, {dt1: [expected_row1],
                                  dt2: [expected_row2]})


class TestSuccessAtomFeed(TestWithConnection):
    async def runTest(self):
        c = await r.connect(host=sharedServerHost,
                            port=sharedServerDriverPort)

        from rethinkdb import ql2_pb2 as p

        if 'success_atom_feed' in (await r.db('test').table_list().run(c)):
            await r.db('test').table_drop('success_atom_feed').run(c)
        await r.db('test').table_create('success_atom_feed').run(c)
        t1 = r.db('test').table('success_atom_feed')

        res = await t1.insert({'id': 0, 'a': 16}).run(c)
        self.assertEqual(res['inserted'], 1)
        res = await t1.insert({'id': 1, 'a': 31}).run(c)
        self.assertEqual(res['inserted'], 1)

        await t1.index_create('a', lambda x: x['a']).run(c)
        await t1.index_wait('a').run(c)

        changes = await t1.get(0).changes(include_initial=True).run(c)
        self.assertTrue(changes.error is None)
        self.assertEqual(len(changes.items), 1)

class TestCursor(TestWithConnection):
    async def setUp(self):
        await TestWithConnection.setUp(self)
        self.conn = await r.connect(host=sharedServerHost,
                                    port=sharedServerDriverPort)

    async def tearDown(self):
        await TestWithConnection.tearDown(self)
        await self.conn.close()
        self.conn = None

    async def test_type(self):
        cursor = await r.range().run(self.conn)
        self.assertTrue(isinstance(cursor, r.Cursor))

    async def test_cursor_after_connection_close(self):
        cursor = await r.range().run(self.conn)
        await self.conn.close()


        async def read_cursor(cursor):
            while (await cursor.fetch_next()):
                await cursor.next()
                cursor.close()

        with self.assertRaisesRegexp(r.ReqlRuntimeError, "Connection is closed."):
             await read_cursor(cursor)

    async def test_cursor_after_cursor_close(self):
        cursor = await r.range().run(self.conn)
        cursor.close()
        count = 0
        while (await cursor.fetch_next()):
            await cursor.next()
            count += 1
        self.assertNotEqual(count, 0, "Did not get any cursor results")

    async def test_cursor_close_in_each(self):
        cursor = await r.range().run(self.conn)
        count = 0

        while (await cursor.fetch_next()):
            await cursor.next()
            count += 1
            if count == 2:
                cursor.close()

        self.assertTrue(count >= 2, "Did not get enough cursor results")

    async def test_cursor_success(self):
        range_size = 10000
        cursor = await r.range().limit(range_size).run(self.conn)
        count = 0
        while (await cursor.fetch_next()):
            await cursor.next()
            count += 1
        self.assertEqual(count, range_size,
             "Expected %d results on the cursor, but got %d" % (range_size, count))

    async def test_cursor_double_each(self):
        range_size = 10000
        cursor = await r.range().limit(range_size).run(self.conn)
        count = 0

        while (await cursor.fetch_next()):
            await cursor.next()
            count += 1
        self.assertEqual(count, range_size,
             "Expected %d results on the cursor, but got %d" % (range_size, count))

        while (await cursor.fetch_next()):
            await cursor.next()
            count += 1
        self.assertEqual(count, range_size,
             "Expected no results on the second iteration of the cursor, but got %d" % (count - range_size))

    # Used in wait tests
    num_cursors=3

    async def do_wait_test(self, wait_time):
        cursors = [ ]
        cursor_counts = [ ]
        cursor_timeouts = [ ]
        for i in xrange(self.num_cursors):
            cur = await r.range().map(r.js("(function (row) {" +
                                                "end = new Date(new Date().getTime() + 2);" +
                                                "while (new Date() < end) { }" +
                                                "return row;" +
                                                "})")).run(self.conn, max_batch_rows=500)
            cursors.append(cur)
            cursor_counts.append(0)
            cursor_timeouts.append(0)

        async def get_next(cursor_index):
            try:
                if wait_time is None: # Special case to use the default
                    await cursors[cursor_index].next()
                else:
                    await cursors[cursor_index].next(wait=wait_time)
                cursor_counts[cursor_index] += 1
            except r.ReqlTimeoutError:
                cursor_timeouts[cursor_index] += 1

        # We need to get ahead of pre-fetching for this to get the error we want
        while sum(cursor_counts) < 4000:
            for cursor_index in xrange(self.num_cursors):
                for read_count in xrange(random.randint(0, 10)):
                    await get_next(cursor_index)

        [cursor.close() for cursor in cursors]

        return (sum(cursor_counts), sum(cursor_timeouts))

    async def test_false_wait(self):
        reads, timeouts = await self.do_wait_test(False)
        self.assertNotEqual(timeouts, 0,
            "Did not get timeouts using zero (false) wait.")

    async def test_zero_wait(self):
        reads, timeouts = await self.do_wait_test(0)
        self.assertNotEqual(timeouts, 0,
            "Did not get timeouts using zero wait.")

    async def test_short_wait(self):
        reads, timeouts = await self.do_wait_test(0.0001)
        self.assertNotEqual(timeouts, 0,
            "Did not get timeouts using short (100 microsecond) wait.")

    async def test_long_wait(self):
        reads, timeouts = await self.do_wait_test(10)
        self.assertEqual(timeouts, 0,
            "Got timeouts using long (10 second) wait.")

    async def test_infinite_wait(self):
        reads, timeouts = await self.do_wait_test(True)
        self.assertEqual(timeouts, 0,
            "Got timeouts using infinite wait.")

    async def test_default_wait(self):
        reads, timeouts = await self.do_wait_test(None)
        self.assertEqual(timeouts, 0,
            "Got timeouts using default (infinite) wait.")

    # This test relies on the internals of the TornadoCursor implementation
    async def test_rate_limit(self):
        # Get the first batch
        cursor = await r.range().run(self.conn)
        cursor_initial_size = len(cursor.items)

        # Wait for the second (pre-fetched) batch to arrive
        await cursor.new_response
        cursor_new_size = len(cursor.items)

        self.assertLess(cursor_initial_size, cursor_new_size)

        # Wait and observe that no third batch arrives
        with self.assertRaises(asyncio.TimeoutError):
            await asyncio.wait_for(asyncio.shield(cursor.new_response), 2)
        self.assertEqual(cursor_new_size, len(cursor.items))

    # Test that an error on a cursor (such as from closing the connection)
    # properly wakes up waiters immediately
    async def test_cursor_error(self):
        cursor = await r.range() \
            .map(lambda row:
                r.branch(row <= 5000, # High enough for multiple batches
                         row,
                         r.js('while(true){ }'))) \
            .run(self.conn)

        async def read_cursor(cursor, hanging):
            try:
                while True:
                    await cursor.next(wait=1)
            except r.ReqlTimeoutError:
                pass
            hanging.set_result(True)
            await cursor.next()

        async def read_wrapper(cursor, done, hanging):
            try:
                with self.assertRaisesRegexp(r.ReqlRuntimeError,
                                             'Connection is closed.'):
                    await read_cursor(cursor, hanging)
                done.set_result(None)
            except Exception as ex:
                if not cursor_hanging.done():
                    cursor_hanging.set_exception(ex)
                done.set_exception(ex)

        cursor_hanging = asyncio.Future()
        done = asyncio.Future()
        asyncio.ensure_future(read_wrapper(cursor, done, cursor_hanging))

        # Wait for the cursor to hit the hang point before we close and cause an error
        await cursor_hanging
        await self.conn.close()
        await done

class TestChangefeeds(TestWithConnection):
    async def setUp(self):
        await TestWithConnection.setUp(self)
        self.conn = await r.connect(host=sharedServerHost,
                                    port=sharedServerDriverPort)
        if 'a' in (await r.db('test').table_list().run(self.conn)):
            await r.db('test').table_drop('a').run(self.conn)
        await r.db('test').table_create('a').run(self.conn)
        if 'b' in (await r.db('test').table_list().run(self.conn)):
            await r.db('test').table_drop('b').run(self.conn)
        await r.db('test').table_create('b').run(self.conn)

    async def tearDown(self):
        await r.db('test').table_drop('b').run(self.conn)
        await r.db('test').table_drop('a').run(self.conn)
        await TestWithConnection.tearDown(self)
        await self.conn.close()
        self.conn = None

    async def table_a_even_writer(self):
        for i in range(10):
            await r.db('test').table("a").insert({"id": i * 2}).run(self.conn)

    async def table_a_odd_writer(self):
        for i in range(10):
            await r.db('test').table("a").insert({"id": i * 2 + 1}).run(self.conn)

    async def table_b_writer(self):
        for i in range(10):
            await r.db('test').table("b").insert({"id": i}).run(self.conn)

    async def cfeed_noticer(self, table, ready, done, needed_values):
        feed = await r.db('test').table(table).changes(squash=False).run(self.conn)
        try:
            ready.set_result(None)
            while len(needed_values) != 0 and (await feed.fetch_next()):
                item = await feed.next()
                self.assertIsNone(item['old_val'])
                self.assertIn(item['new_val']['id'], needed_values)
                needed_values.remove(item['new_val']['id'])
            done.set_result(None)
        except Exception as ex:
            done.set_exception(ex)

    async def test_multiple_changefeeds(self):
        loop = asyncio.get_event_loop()
        feeds_ready = { }
        feeds_done = { }
        needed_values = { 'a': set(range(20)), 'b': set(range(10)) }
        for n in ('a', 'b'):
            feeds_ready[n] = asyncio.Future()
            feeds_done[n] = asyncio.Future()
            asyncio.ensure_future(self.cfeed_noticer(n, feeds_ready[n], feeds_done[n],
                                             needed_values[n]))

        await asyncio.wait(feeds_ready.values())
        await asyncio.wait([self.table_a_even_writer(),
               self.table_a_odd_writer(),
               self.table_b_writer()])
        await asyncio.wait(feeds_done.values())
        self.assertTrue(all([len(x) == 0 for x in needed_values.values()]))

if __name__ == '__main__':
    print("Running asyncio connection tests")
    suite = unittest.TestSuite()
    loader = unittest.TestLoader()
    suite.addTest(loader.loadTestsFromTestCase(TestCursor))
    suite.addTest(loader.loadTestsFromTestCase(TestChangefeeds))
    suite.addTest(loader.loadTestsFromTestCase(TestNoConnection))
    suite.addTest(loader.loadTestsFromTestCase(TestConnection))
    # suite.addTest(TestBatching())
    # suite.addTest(TestGetIntersectingBatching())
    # suite.addTest(TestGroupWithTimeKey())
    # suite.addTest(TestSuccessAtomFeed())
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
