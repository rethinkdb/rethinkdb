from __future__ import print_function

import atexit, itertools, os, re, sys, time, warnings
from datetime import datetime, tzinfo, timedelta

stashedPath = sys.path
sys.path.insert(0, os.path.realpath(os.path.join(os.path.dirname(__file__), os.path.pardir, os.path.pardir, 'common')))
import driver, utils
sys.path = stashedPath

try:
    unicode
except NameError:
    unicode = str
try:
	xrange
except NameError:
	xrange = range
try:
    long
except NameError:
    long = int
try:
    izip_longest = itertools.izip_longest
except AttributeError:
    izip_longest = itertools.zip_longest

# -- global variables

failure_count = 0
passed_count = 0
start_time=time.time()

# -- timezone objects

class UTCTimeZone(tzinfo):
    '''UTC'''
    
    def utcoffset(self, dt):
        return timedelta(0)
    
    def tzname(self, dt):
        return "UTC"
    
    def dst(self, dt):
        return timedelta(0)

class PacificTimeZone(tzinfo):
    '''Pacific timezone emulator for timestamp: 1375147296.68'''
    
    def utcoffset(self, dt):
        return timedelta(-1, 61200)
    
    def tzname(self, dt):
        return 'PDT'
    
    def dst(self, dt):
        return timedelta(0, 3600)

# -- import driver

r = utils.import_python_driver()
print('Using RethinkDB client from: %s' % r.__file__)

# -- get settings

DEBUG_ENABLED = os.environ.get('VERBOSE', 'false').lower() == 'true'

def print_debug(message):
    if DEBUG_ENABLED:
        print('DEBUG (%.2f):\t %s' % (time.time() - start_time, message.rstrip()))
        sys.stdout.flush()

DRIVER_PORT = int(sys.argv[1] if len(sys.argv) > 1 else os.environ.get('RDB_DRIVER_PORT'))
print_debug('Using driver port: %d' % DRIVER_PORT)

required_external_tables = []
if len(sys.argv) > 2 or os.environ.get('TEST_DB_AND_TABLE_NAME'):
    for rawValue in (sys.argv[2] if len(sys.argv) > 3 else os.environ.get('TEST_DB_AND_TABLE_NAME')).split(','):
        rawValue = rawValue.strip()
        if rawValue == '':
            continue
        splitValue = rawValue.split('.')
        if len(splitValue) == 1:
            required_external_tables += [('test', splitValue[0])]
        elif len(splitValue) == 2:
            required_external_tables += [(splitValue[0], splitValue[1])]
        else:
            raise AssertionError('Unuseable value for external tables: %s' % rawValue)
required_external_tables.reverse() # setup for .pop()

# -- utilities --

def print_test_failure(test_name, test_src, message):
    global failure_count
    failure_count += 1
    print('')
    print("TEST FAILURE: %s" % test_name.encode('utf-8'))
    print("   TEST BODY: %s" % test_src.encode('utf-8'))
    print("   " + message)
    print('')

def check_pp(src, query):
    # This isn't a good indicator because of lambdas, whitespace differences, etc
    # But it will at least make sure that we don't crash when trying to print a query
    printer = r.errors.QueryPrinter(query)
    composed = printer.print_query()
    #if composed != src:
    #    print('Warning, pretty printing inconsistency:')
    #    print("Source code: %s", src)
    #    print("Printed query: %s", composed)

class Lst:
    def __init__(self, lst, partial=None, **kwargs):
        self.lst = lst
        self.kwargs = kwargs

    def __eq__(self, other):
        if not hasattr(other, '__iter__'):
            return False

        for otherItem, selfItem in izip_longest(other, self.lst, fillvalue=self.__class__):
            if self.__class__ in (otherItem, selfItem):
                return False # mismatched lengths
            if not eq(selfItem, **self.kwargs)(otherItem):
                return False

        return True

    def __repr__(self):
        return repr(self.lst)

class Bag(Lst):
    '''Compare array without repect to order'''
    
    # note: This only works for dicts, arrays, numbers, Nones, and strings. Anything else might as well be random.

    def __init__(self, lst, partial=False, **kwargs):
        self.lst = sorted(lst, key=lambda x: repr(x))
        self.partial = partial == True
        self.kwargs = kwargs

    def __eq__(self, other):
        if not hasattr(other, '__iter__'):
            return False
        
        other = sorted(other, key=lambda x: repr(x))
        
        if self.partial:
            if len(self.lst) > len(other):
                return False
            
            otherIter = iter(other)
            for mine in self.lst:
                for theirs in otherIter:
                    if eq(mine, **self.kwargs)(theirs):
                        break
                else:
                    return False
        else:
            if len(self.lst) != len(other):
                return False
        
            for a, b in zip(self.lst, other):
                if not eq(a, **self.kwargs)(b):
                    return False
        
        return True

class Dct:
    def __init__(self, dct, partial=False, **kwargs):
        assert isinstance(dct, dict)
        self.dct = dct
        self.partial = partial == True
        self.kwargs = kwargs

    def __eq__(self, other):
        if not isinstance(other, dict):
            return False
        
        if self.partial is True:
            if not set(self.keys()).issubset(set(other.keys())):
                return False
        else:
            if not set(self.keys()) == set(other.keys()):
                return False
        
        for key in self.dct:
            if not key in other:
                return False
            val = other[key]
            if isinstance(val, (str, unicode)):
                # Remove additional error info that creeps in in debug mode
                val = re.sub("(?ms)\nFailed assertion:.*", "", val)
            other[key] = val
            if not eq(self.dct[key], **self.kwargs)(other[key]):
                return False
        return True
    
    def keys(self):
        return self.dct.keys()
    
    def __repr__(self):
        return repr(self.dct)

class Err:
    inRegex = re.compile('^(?P<message>[^\n]*?)(?: in)?:\n.*$', flags=re.DOTALL)
    assertionRegex = re.compile('^(?P<message>[^\n]*?)\nFailed assertion:.*$', flags=re.DOTALL)
    
    def __init__(self, err_type=None, message=None, err_frames=None, regex=False, **kwargs):
        
        # translate err_type into the class
        if type(err_type) == type(Exception) and issubclass(err_type, Exception):
            self.err_type = err_type
        elif hasattr(r, err_type):
            self.err_type = r.__getattribute__(err_type)
        elif hasattr(__builtins__, err_type):
            self.err_type = __builtins__.__getattribute__(err_type)
        assert issubclass(self.err_type, Exception), 'err_type must be a subclass Exception, got: %r' % err_type
        
        self.message = message
        self.frames = None # err_frames # TODO: test frames
        self.regex = regex

    def __eq__(self, other):
        
        # -- type
        
        if not isinstance(other, self.err_type):
            return False
        
        # -- message
        
        # Strip details from output message
        otherMessage = None
        with warnings.catch_warnings():
            warnings.simplefilter('ignore')
            if hasattr(other, 'message'):
                otherMessage = str(other.message)
            else:
                otherMessage = str(other)
        otherMessage = re.sub(self.inRegex, '\g<message>:', otherMessage)
        otherMessage = re.sub(self.assertionRegex, '\g<message>', otherMessage)
        
        if self.regex:
            if not re.match(unicode(self.message), otherMessage):
                return False
        else:
            if not self.message == otherMessage:
                return False
        
        # -- frames
        
        if self.frames and (not hasattr(other, 'frames') or self.frames != other.frames):
                return False
        
        # --
        
        return True

    def __repr__(self):
        return "%s(%s'%s')" % (self.err_type.__name__, self.regex and '~' or '', str(self.message) or '')


class Arr:
    def __init__(self, length, thing=None, partial=None, **kwargs):
        self.length = length
        self.thing = thing
        self.kwargs = kwargs

    def __eq__(self, arr):
        if not isinstance(arr, list):
            return False

        if not self.length == len(arr):
            return False

        if self.thing is None:
            return True

        return all([eq(v, **self.kwargs)(self.thing) for v in arr])

    def __repr__(self):
        return "arr(%d, %s)" % (self.length, repr(self.thing))

class Uuid:
    def __init__(self, **kwargs):
        pass
    
    def __eq__(self, thing):
        if not isinstance(thing, (str, unicode)):
            return False
        return re.match("[a-z0-9]{8}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{12}", thing) != None

    def __repr__(self):
        return "uuid()"

class Number:
    def __init__(self, value, precision=0.0, explicit_type=None, **kwargs):
        if explicit_type is not None:
            if not isinstance(value, explicit_type):
                raise ValueError('Number with an explicit type (%s) got an incorrect value: %s' % (str(explicit_type), str(value)))
        elif not isinstance(value, (float, int, long)):
            raise ValueError('Number got a non-numeric value: %s (%s)' % (str(value), type(value)))
        self.value = value
        self.precision = precision
        self.explicit_type = explicit_type

    def __eq__(self, other):
        testType = self.explicit_type or (float, int, long)
        if not isinstance(other, testType):
            return False
        return abs(self.value - other) <= self.precision
    
    def __repr__(self):
        return "%s: %s" % (type(self.value).__name__, str(self.value))

class Regex:
    value = None

    def __init__ (self, value, **kwargs):
        try:
            self.value = re.compile(value)
        except Exception as e:
            raise ValueError('Regex got a bad value: %r' % value)
    
    def __eq__(self, other):
        if not isinstance(other, (str, unicode)):
            return False
        return not self.value.match(other) is None
    
    def __repr__(self):
        return "Regex: %s" % (self.value.pattern if self.value else '<none>')

# -- Curried output test functions --

def eq(exp, **kwargs):
    if exp == ():
        return lambda x: True

    if isinstance(exp, list):
        exp = Lst(exp, **kwargs)
    elif isinstance(exp, dict):
        exp = Dct(exp, **kwargs)
    elif isinstance(exp, (float, int, long)):
        exp = Number(exp, **kwargs)
    
    return lambda val: val == exp

class PyTestDriver:
    
    cpp_conn = None
    
    def __init__(self):
        print('Creating default connection to server on port %d\n' % DRIVER_PORT)
        self.cpp_conn = self.connect()
        self.scope = globals()
        self.scope['conn'] = self.cpp_conn # allow access to connection
    
    def connect(self):
        return r.connect(host='localhost', port=DRIVER_PORT)

    def define(self, expr, variable):
        print_debug('Defining: %s%s' % (expr, ' to %s' % variable if variable else ''))
        try:
            exec(compile('%s = %s' % (variable, expr), '<string>', 'single'), self.scope) # handle things like: a['b'] = b
        except Exception as e:
            print_test_failure('Exception while processing define', expr, str(e))
    
    def run(self, src, expected, name, runopts, testopts):
        global passed_count
        
        print_debug('Test: %s' % name)
        
        if runopts:
            runopts["profile"] = True
        else:
            runopts = {"profile": True}
        
        compOptions = {}
        if 'precision' in testopts:
            compOptions['precision'] = float(testopts['precision']) # errors will bubble up
        
        conn = None
        if 'new-connection' in testopts and testopts['new-connection'] is True:
            conn = self.connect()
        else:
            conn = self.cpp_conn
        
        # -- build the expected result
        
        if expected:
            exp_val = eval(expected, self.scope)
        else:
            # This test might not have come with an expected result, we'll just ensure it doesn't fail
            exp_val = ()
        print_debug('\tExpected: %s' % str(expected))
        
        # -- evaluate the command
        
        try:
            result = eval(src, self.scope)
        except Exception as err:
            print_debug('\tError evaluating: %s - %r' % (src, err))
            result = err
        else:
            try:
                # - collect the contents of a cursor
                
                if isinstance(result, r.Cursor):
                    print_debug('\tEvaluating cursor: %s %r' % (src, runopts))
                    result = list(result)
                
                # - run as a query if it is one
                
                elif isinstance(result, r.RqlQuery):
                    print_debug('\tRunning query: %s %r' % (src, runopts))
                    
                    # Check pretty-printing
                    
                    check_pp(src, result)
                    
                    # run the query
                    
                    result = result.run(conn, **runopts)
                    if result and "profile" in runopts and runopts["profile"] and "value" in result:
                        result = result["value"]
                    # ToDo: do something reasonable with the profile
                
                else:
                    print_debug('\tRunning: %s' % src)
                
                # - Save variable if requested
                
                if 'variable' in testopts:
                    # ToDo: handle complex variables like: a[2]
                    self.scope[testopts['variable']] = result
                    print_debug('\tVariable: %s' % testopts['variable'])
                    if exp_val is None:
                        return
    
                if 'noreply_wait' in testopts and testopts['noreply_wait']:
                    conn.noreply_wait()
            
            except Exception as err:
                print_debug('\tError: %r' % err)
                result = err
            else:
                print_debug('\tResult: %r' % result)
        
        # Compare to the expected result
        
        if isinstance(result, Exception):
            if not isinstance(exp_val, Err):
                print_test_failure(name, src, "Error running test on server:\n\tERROR:    %r\n\tEXPECTED: %r" % (result, exp_val))
            elif not eq(exp_val, **compOptions)(result):
                print_test_failure(name, src, "Error result not equal to expected err:\n\tERROR:    %r\n\tEXPECTED: %r" % (result, exp_val))
            else:
                passed_count += 1
        elif not eq(exp_val, **compOptions)(result):
            print_test_failure(name, src, "Result is not equal to expected result:\n\tVALUE:    %r\n\tEXPECTED: %r" % (result, exp_val))
        else:
            passed_count += 1

driver = PyTestDriver()

# Emitted test code will consist of calls to these functions

def test(query, expected, name, runopts=None, testopts=None):
    if runopts is None:
        runopts = {}
    else:
        for k, v in runopts.items():
            if isinstance(v, str):
                runopts[k] = eval(v)
    if testopts is None:
        testopts = {}
    
    if 'max_batch_rows' not in runopts:
        runopts['max_batch_rows'] = 3
    if expected == '':
        expected = None
    driver.run(query, expected, name, runopts, testopts)

def setup_table(table_variable_name, table_name, db_name='test'):
    global required_external_tables
    
    def _teardown_table(table_name, db_name):
        '''Used for tables that get created for this test'''
        res = r.db(db_name).table_drop(table_name).run(driver.cpp_conn)
        assert res["tables_dropped"] == 1, 'Failed to delete table %s.%s: %s' % (db_name, table_name, str(res))
    
    def _clean_table(table_name, db_name):
        '''Used for pre-existing tables'''
        res = r.db(db_name).table(table_name).delete().run(driver.cpp_conn)
        assert res["errors"] == 0, 'Failed to clean out contents from table %s.%s: %s' % (db_name, table_name, str(res))
        r.db(db_name).table(table_name).index_list().for_each(r.db(db_name).table(table_name).index_drop(r.row)).run(driver.cpp_conn)
    
    if len(required_external_tables) > 0:
        db_name, table_name = required_external_tables.pop()
        try:
            r.db(db_name).table(table_name).info(driver.cpp_conn)
        except r.ReqlRuntimeError:
            raise AssertionError('External table %s.%s did not exist' % (db_name, table_name))
        atexit.register(_clean_table, table_name=table_name, db_name=db_name)
        
        print('Using existing table: %s.%s, will be %s' % (db_name, table_name, table_variable_name))
    else:
        if table_name in r.db(db_name).table_list().run(driver.cpp_conn):
            r.db(db_name).table_drop(table_name).run(driver.cpp_conn)
        res = r.db(db_name).table_create(table_name).run(driver.cpp_conn)
        assert res["tables_created"] == 1, 'Unable to create table %s.%s: %s' % (db_name, table_name, str(res))
        r.db(db_name).table(table_name).wait().run(driver.cpp_conn)
        
        print_debug('Created table: %s.%s, will be %s' % (db_name, table_name, table_variable_name))
    
    globals()[table_variable_name] = r.db(db_name).table(table_name)

def setup_table_check():
    '''Make sure that the required tables have been setup'''
    if len(required_external_tables) > 0:
        raise Exception('Unused external tables, that is probably not supported by this test: %s' % ('%s.%s' % tuple(x) for x in required_external_tables).join(', '))

def check_no_table_specified():
    if DB_AND_TABLE_NAME != "no_table_specified":
        raise ValueError("This test isn't meant to be run against a specific table")

def define(expr, variable=None):
    driver.define(expr, variable=variable)

def bag(lst):
    return Bag(lst)

def partial(expected):
    if hasattr(expected, 'keys'):
        return Dct(expected, partial=True)
    elif hasattr(expected, '__iter__'):
        return Bag(expected, partial=True)
    else:
        raise ValueError('partial can only work on dicts or iterables, got: %s (%s)' % (type(expected).__name__, str(expected)))

def fetch(cursor, limit=None):
    '''Pull items from a cursor'''
    if limit is not None:
        try:
            limit = int(limit)
            assert limit > 0
        except Exception:
            "On fetch limit must be None or > 0, got: %s" % str(limit)
    result = []
    for i, value in enumerate(cursor, start=1):
        result.append(value)
        if limit and i >= limit:
            break
    return result

def wait(seconds):
    '''Sleep for some seconds'''
    time.sleep(seconds)

def err(err_type, message=None, frames=None):
    return Err(err_type, message, frames)

def err_regex(err_type, message=None, frames=None):
    return Err(err_type, message, frames, True)

def arrlen(length, thing=None):
    return Arr(length, thing)

def uuid():
    return Uuid()

def regex(value):
    return Regex(value)

def int_cmp(expected_value):
    return Number(expected_value, explicit_type=(int, long))

def float_cmp(expected_value):
    return Number(expected_value, explicit_type=float)

def the_end():
    if failure_count > 0:
        sys.exit("Failed %d tests, passed %d" % (failure_count, passed_count))
    else:
        print("Passed all %d tests" % passed_count)

false = False
true = True
