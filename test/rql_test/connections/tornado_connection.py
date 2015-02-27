#!/usr/bin/env python
##
# Tests the driver API for making connections and excercizes the
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
import traceback
import unittest
import functools
from tornado import gen, ioloop

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

# -- use tornado subdriver

r.set_loop_type("tornado")

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


@gen.coroutine
def checkSharedServer():
    if sharedServerDriverPort is not None:
        conn = yield r.aconnect(host=sharedServerHost,
                                port=sharedServerDriverPort)
        if 'test' not in (yield r.db_list().run(conn)):
            yield r.db_create('test').run(conn)


@gen.coroutine
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
    ioloop.IOLoop.instance()\
                 .run_sync(functools.partial(coroutine, *args, **kwargs))


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
            raise AssertionError('%s not greater than or equal to %s'
                                 % (greater, lesser))

    def replacement_assertLess(self, lesser, greater):
        if not greater > lesser:
            raise AssertionError('%s not less than %s' % (lesser, greater))

    def replacement_skipTest(self, message):
        sys.stderr.write("%s " % message)

    def replacement_assertRaisesRegexp(self, exception, regexp,
                                       callable_func, *args, **kwds):
        try:
            callable_func(*args, **kwds)
        except Exception as e:
            self.assertTrue(isinstance(e, exception),
                            '%s expected to raise %s but '
                            'instead raised %s: %s\n%s'
                            % (repr(callable_func), repr(exception),
                               e.__class__.__name__, str(e),
                               traceback.format_exc()))
            self.assertTrue(re.search(regexp, str(e)),
                            '%s did not raise the expected '
                            'message "%s", but rather: %s'
                            % (repr(callable_func), str(regexp), str(e)))
        else:
            self.fail('%s failed to raise a %s'
                      % (repr(callable_func), repr(exception)))


class TestWithConnection(TestCaseCompatible):
    port = None
    server = None
    serverOutput = None
    ioloop = None

    @gen.coroutine
    def setUp(self):
        global sharedServer, sharedServerOutput, sharedServerHost, \
            sharedServerDriverPort

        if sharedServer is not None:
            try:
                yield sharedServer.check()
            except Exception:
                # ToDo: figure out how to blame the last test
                yield closeSharedServer()

        if sharedServerDriverPort is None:
            sharedServerOutput = tempfile.NamedTemporaryFile('w+')
            sharedServer = driver.Process(executable_path=rethinkdb_exe,
                                          console_output=sharedServerOutput,
                                          wait_until_ready=True)
            sharedServerHost = sharedServer.host
            sharedServerDriverPort = sharedServer.driver_port

        # - insure we are ready

        yield checkSharedServer()

    # can't use standard TestCase run here because async.
    def run(self, result=None):
        return just_do(self.arun, result)

    @gen.coroutine
    def arun(self, result=None):
        if result is None:
            result = self.defaultTestResult()
        result.startTest(self)
        testMethod = getattr(self, self._testMethodName)
        try:
            try:
                yield self.setUp()
            except KeyboardInterrupt:
                raise
            except:
                result.addError(self, self._exc_info())
                return

            ok = False
            try:
                yield testMethod()
                ok = True
            except self.failureException:
                result.addFailure(self, self._exc_info())
            except KeyboardInterrupt:
                raise
            except:
                result.addError(self, self._exc_info())

            try:
                yield self.tearDown()
            except KeyboardInterrupt:
                raise
            except:
                result.addError(self, self._exc_info())
                ok = False
            if ok:
                result.addSuccess(self)
        finally:
            result.stopTest(self)

    @gen.coroutine
    def asyncAssertRaisesRegexp(self, exception, regexp, generator):
        try:
            yield generator
        except Exception as e:
            self.assertTrue(isinstance(e, exception),
                            '%s expected to raise %s but '
                            'instead raised %s: %s\n%s'
                            % (repr(generator), repr(exception),
                               e.__class__.__name__, str(e),
                               traceback.format_exc()))
            self.assertTrue(re.search(regexp, str(e)),
                            '%s did not raise the expected '
                            'message "%s", but rather: %s'
                            % (repr(generator), str(regexp), str(e)))
        else:
            self.fail('%s failed to raise a %s'
                      % (repr(generator), repr(exception)))

    @gen.coroutine
    def asyncAssertRaises(self, exception, generator):
        try:
            yield generator
        except Exception as e:
            self.assertTrue(isinstance(e, exception),
                            '%s expected to raise %s but '
                            'instead raised %s: %s\n%s'
                            % (repr(generator), repr(exception),
                               e.__class__.__name__, str(e),
                               traceback.format_exc()))
        else:
            self.fail('%s failed to raise a %s'
                      % (repr(generator), repr(exception)))

    @gen.coroutine
    def tearDown(self):
        global sharedServer, sharedServerOutput, sharedServerHost, \
            sharedServerDriverPort

        if sharedServerDriverPort is not None:
            try:
                yield checkSharedServer()
            except Exception:
                yield closeSharedServer()
                raise  # ToDo: figure out how to best give the server log

# == Test Classes


class TestConnection(TestWithConnection):
    @gen.coroutine
    def test_connect_close_reconnect(self):
        c = yield r.aconnect(host=sharedServerHost,
                             port=sharedServerDriverPort)
        yield r.expr(1).run(c)
        yield c.aclose()
        yield c.aclose()
        yield c.reconnect()
        yield r.expr(1).run(c)

    @gen.coroutine
    def test_connect_close_expr(self):
        c = yield r.aconnect(host=sharedServerHost,
                             port=sharedServerDriverPort)
        yield r.expr(1).run(c)
        yield c.aclose()
        yield self.asyncAssertRaisesRegexp(r.RqlDriverError,
                                           "Connection is closed.",
                                           r.expr(1).run(c))

    @gen.coroutine
    def test_noreply_wait_waits(self):
        c = yield r.aconnect(host=sharedServerHost,
                             port=sharedServerDriverPort)
        t = time.time()
        yield r.js('while(true);', timeout=0.5).run(c, noreply=True)
        yield c.noreply_wait()
        duration = time.time() - t
        self.assertGreaterEqual(duration, 0.5)

    @gen.coroutine
    def test_close_waits_by_default(self):
        c = yield r.aconnect(host=sharedServerHost,
                             port=sharedServerDriverPort)
        t = time.time()
        yield r.js('while(true);', timeout=0.5).run(c, noreply=True)
        yield c.aclose()
        duration = time.time() - t
        self.assertGreaterEqual(duration, 0.5)

    @gen.coroutine
    def test_reconnect_waits_by_default(self):
        c = yield r.aconnect(host=sharedServerHost,
                             port=sharedServerDriverPort)
        t = time.time()
        yield r.js('while(true);', timeout=0.5).run(c, noreply=True)
        yield c.reconnect()
        duration = time.time() - t
        self.assertGreaterEqual(duration, 0.5)

    @gen.coroutine
    def test_close_does_not_wait_if_requested(self):
        c = yield r.aconnect(host=sharedServerHost,
                             port=sharedServerDriverPort)
        t = time.time()
        yield r.js('while(true);', timeout=0.5).run(c, noreply=True)
        yield c.aclose(noreply_wait=False)
        duration = time.time() - t
        self.assertLess(duration, 0.5)

    @gen.coroutine
    def test_reconnect_does_not_wait_if_requested(self):
        c = yield r.aconnect(host=sharedServerHost,
                             port=sharedServerDriverPort)
        t = time.time()
        yield r.js('while(true);', timeout=0.5).run(c, noreply=True)
        yield c.reconnect(noreply_wait=False)
        duration = time.time() - t
        self.assertLess(duration, 0.5)

    @gen.coroutine
    def test_db(self):
        c = yield r.aconnect(host=sharedServerHost,
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
        yield self.asyncAssertRaisesRegexp(r.RqlRuntimeError,
                                           "Table `db2.t1` does not exist.",
                                           r.table('t1').run(c))

        c.use('test')
        yield r.table('t1').run(c)
        yield self.asyncAssertRaisesRegexp(r.RqlRuntimeError,
                                           "Table `test.t2` does not exist.",
                                           r.table('t2').run(c))

        yield c.aclose()

        # Test setting the db in connect
        c = yield r.aconnect(db='db2', host=sharedServerHost,
                             port=sharedServerDriverPort)
        yield r.table('t2').run(c)

        yield self.asyncAssertRaisesRegexp(r.RqlRuntimeError,
                                           "Table `db2.t1` does not exist.",
                                           r.table('t1').run(c))

        yield c.aclose()

        # Test setting the db as a `run` option
        c = yield r.aconnect(host=sharedServerHost,
                             port=sharedServerDriverPort)
        yield r.table('t2').run(c, db='db2')

    @gen.coroutine
    def test_use_outdated(self):
        c = yield r.aconnect(host=sharedServerHost,
                             port=sharedServerDriverPort)

        if 't1' in (yield r.db('test').table_list().run(c)):
            yield r.db('test').table_drop('t1').run(c)
        yield r.db('test').table_create('t1').run(c)

        # Use outdated is an option that can be passed to db.table or `run`
        # We're just testing here if the server actually accepts the option.

        yield r.table('t1', use_outdated=True).run(c)
        yield r.table('t1').run(c, use_outdated=True)

    @gen.coroutine
    def test_repl(self):
        # Calling .repl() should set this connection as global state
        # to be used when `run` is not otherwise passed a connection.
        c = (yield r.aconnect(host=sharedServerHost,
                              port=sharedServerDriverPort)).repl()

        yield r.expr(1).run()

        c.repl()                # is idempotent

        yield r.expr(1).run()

        yield c.aclose()

        yield self.asyncAssertRaisesRegexp(r.RqlDriverError,
                                           "Connection is closed",
                                           r.expr(1).run())

    @gen.coroutine
    def test_port_conversion(self):
        c = yield r.aconnect(host=sharedServerHost,
                             port=str(sharedServerDriverPort))
        yield r.expr(1).run(c)
        yield c.aclose()

        yield self.asyncAssertRaisesRegexp(r.RqlDriverError,
                                           "Could not convert port abc to an integer.",
                                           r.aconnect(port='abc',
                                                      host=sharedServerHost))


class TestShutdown(TestWithConnection):
    @gen.coroutine
    def setUp(self):
        if sharedServer is None:
            # we need to be able to kill the server, so can't use one
            # from outside
            yield closeSharedServer()
        yield super(TestShutdown, self).setUp()

    @gen.coroutine
    def test_shutdown(self):
        c = yield r.aconnect(host=sharedServerHost,
                             port=sharedServerDriverPort)
        yield r.expr(1).run(c)

        yield closeSharedServer()
        yield gen.sleep(0.2)

        yield self.asyncAssertRaisesRegexp(r.RqlDriverError,
                                           "Connection interrupted receiving.*.",
                                           r.expr(1).run(c))


# Another non-connection connection test. It's to test that get_intersecting()
# batching works properly.
class TestGetIntersectingBatching(TestWithConnection):
    @gen.coroutine
    def runTest(self):
        c = r.aconnect(host=sharedServerHost,
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
            reference = t1.filter(r.row['geo'].intersects(query_circle))\
                          .coerce_to("ARRAY").run(c)
            cursor = yield t1.get_intersecting(query_circle, index='geo')\
                             .run(c, max_batch_rows=batch_size)
            if not cursor.end_flag:
                seen_lazy = True

            itr = iter(cursor)
            while len(reference) > 0:
                row = yield next(itr)
                self.assertEqual(reference.count(row), 1)
                reference.remove(row)
            self.asyncAssertRaises(StopIteration, next(itr))
            self.assertTrue(cursor.end_flag)

        self.assertTrue(seen_lazy)

        yield r.db('test').table_drop('t1').run(c)


class TestBatching(TestWithConnection):
    @gen.coroutine
    def runTest(self):
        c = yield r.aconnect(host=sharedServerHost,
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

        itr = iter(cursor)
        for i in xrange(0, count - 1):
            row = yield next(itr)
            ids.remove(row['id'])

        self.assertEqual((yield next(itr)['id']), ids.pop())
        self.asyncAssertRaises(StopIteration, next(itr))
        self.assertTrue(cursor.end_flag)
        yield r.db('test').table_drop('t1').run(c)


class TestGroupWithTimeKey(TestWithConnection):
    @gen.coroutine
    def runTest(self):
        c = r.aconnect(host=sharedServerHost,
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
    @gen.coroutine
    def runTest(self):
        c = yield r.aconnect(host=sharedServerHost,
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

        self.assertEqual(p.Response.ResponseType.SUCCESS_ATOM_FEED,
                         (yield t1.get(0).changes().run(c).responses[0].type))


if __name__ == '__main__':
    print("Running py connection tests")
    suite = unittest.TestSuite()
    loader = unittest.TestLoader()
    suite.addTest(loader.loadTestsFromTestCase(TestConnection))
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
        sys.stderr.write('The server did not close cleanly after testing: %s'
                         % str(e))

    if not res.wasSuccessful() or not serverClosedCleanly:
        sys.exit(1)
