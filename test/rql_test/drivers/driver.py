from __future__ import print_function

import atexit, copy, inspect, itertools, os, re, sys, time, warnings
from datetime import datetime, tzinfo, timedelta # used by time tests

stashedPath = copy.copy(sys.path)
sys.path.insert(0, os.path.realpath(os.path.join(os.path.dirname(__file__), os.path.pardir, os.path.pardir, 'common')))
import driver, utils
sys.path = stashedPath

try:
    unicode
except NameError:
    unicode = str
try:
    long
except NameError:
    long = int

# -- global variables

failure_count = 0
passed_count = 0
start_time=time.time()

# -- import driver

r = utils.import_python_driver()
print('Using RethinkDB client from: %s' % r.__file__)

# -- get settings

DEBUG_ENABLED = os.environ.get('VERBOSE', 'false').lower() == 'true'

def print_debug(message):
    if DEBUG_ENABLED:
        print('DEBUG (%.2f):\t %s' % (time.time() - start_time, message.rstrip()))
        sys.stdout.flush()

DRIVER_PORT = int(sys.argv[1] if len(sys.argv) > 1 else os.environ.get('RDB_DRIVER_PORT') or 28015)
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

def print_failure(name, src, expected, result, message=None):
    global failure_count
    failure_count += 1
    print('''
TEST FAILURE: %(name)s%(message)s
    SOURCE:   %(source)s
    EXPECTED: %(expected)s
    RESULT:   %(result)s''' % {
        'name':     name,
        'source':   src,
        'message':  '\n    FAILURE:  %s' % message if message is not None else '',
        'expected': expected,
        'result':   result
    })

def check_pp(src, query):
    # This isn't a good indicator because of lambdas, whitespace differences, etc
    # But it will at least make sure that we don't crash when trying to print a query
    printer = r.errors.QueryPrinter(query)
    composed = printer.print_query()
    #if composed != src:
    #    print('Warning, pretty printing inconsistency:')
    #    print("Source code: %s", src)
    #    print("Printed query: %s", composed)

class OptionsBox():
    value = None
    options = None
    
    def __init__(self, value, options):
        assert isinstance(options, dict)
        self.value = value
        self.options = options
    
    def __str__(self):
        if self.options and self.options.keys() == ['ordered'] and self.options['ordered'] == False:
            return 'bag(%s)' % self.value
        elif self.options and self.options.keys() == ['partial'] and self.options['partial'] == True:
            return 'partial(%s)' % self.value
        else:
            return 'options(%s, %s)' % (self.options, self.value)
    
    def __repr__(self):
        if self.options and self.options.keys() == ['ordered'] and self.options['ordered'] == False:
            return 'bag(%r)' % self.value
        elif self.options and self.options.keys() == ['partial'] and self.options['partial'] == True:
            return 'partial(%r)' % self.value
        else:
            return 'options(%s, %r)' % (self.options, self.value)

class FalseStr(str):
    '''return a string that evaluates as false'''
    def __nonzero__(self):
        return False

class Anything():
    __instance = None
    
    def __new__(cls):
        if not cls.__instance:
            cls.__instance = super(Anything, cls).__new__(cls)
        return cls.__instance 
    
    def __str__(self):
        return "anything()"
    
    def __repr__(self):
        return self.__str__()

class Err():
    exceptionRegex = re.compile('^(?P<message>[^\n]*?)((?: in)?:\n|\nFailed assertion:).*$', flags=re.DOTALL)
    
    err_type = None
    message = None
    frames = None
    regex = False
    
    def __init__(self, err_type=None, message=None, err_frames=None, **kwargs):
        
        # translate err_type into the class
        if type(err_type) == type(Exception) and issubclass(err_type, Exception):
            self.err_type = err_type
        elif hasattr(r, err_type):
            self.err_type = r.__getattribute__(err_type)
        elif hasattr(__builtins__, err_type):
            self.err_type = __builtins__.__getattribute__(err_type)
        else:
            try:
                self.err_type = eval(err_type) # just in case we got a string with the name
            except Exception: pass
        assert issubclass(self.err_type, Exception), 'err_type must be a subclass Exception, got: %r' % err_type
        
        if message is not None:
            self.message = message
        
        self.frames = None # TODO: test frames
    
    def __str__(self):
        return "%s(%s)" % (self.err_type.__name__, ('~' + self.message.pattern) if hasattr(self.message, 'pattern') else self.message)
    
    def __repr__(self):
        return self.__str__()

class Regex:
    value = None

    def __init__ (self, value, **kwargs):
        try:
            self.value = re.compile(value)
        except Exception as e:
            raise ValueError('Regex got a bad value: %r' % value)
    
    def match(self, other):
        if not isinstance(other, (str, unicode)):
            return False
        return self.value.match(other) is not None
        
    @property
    def pattern(self):
        return self.value.pattern
    
    def __str__(self):
        return "Regex(%s)" % (self.value.pattern if self.value else '<none>')
    
    def __repr__(self):
        return "Regex(%s)" % (self.value.pattern if self.value else '<none>')

class Uuid(Regex):
    value = re.compile('^[a-z0-9]{8}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{12}$')
    
    def __init__(self, **kwargs):
        pass
    
    def __str__(self):
        return "uuid()"
    
    def __repr__(self):
        return "uuid()"

def compare(expected, result, options=None):
    '''Compare the two items by the rules we have, returning either True, or a message about why it failed'''
    # -- merge options
    
    defaultOptions = {
        'ordered':       True,
        'partial':       False,
        'precision':     0.0,
        'explicit_type': None
    }
    if options is None:
        options = defaultOptions
    else:
        options = copy.copy(options)
        for key in defaultOptions:
            if key == 'explicit_type':
                options[key] = None
            if key not in options:
                options[key] = defaultOptions[key]
    
    if isinstance(expected, OptionsBox) and expected.options:
        for key in defaultOptions:
            if key in expected.options:
                options[key] = expected.options[key]
        expected = expected.value
    
    # == compare based on type of expected
    
    if inspect.isclass(expected):
        try:
            expected = expected()
        except Exception as e:
            return FalseStr('Expected was a class that can not easily be instantiated: %s' % str(e))
    
    # -- explicit type
    if options['explicit_type'] and not isinstance(result, options['explicit_type']):
        return FalseStr('expected explicit type %s, got %s (%s)' % (options['explicit_type'], result, type(result).__name__))
    
    # -- Anything... but an error
    if isinstance(expected, Anything):
        if isinstance(result, Exception):
            return FalseStr('expected anything() but got error: %r' % result)
        else:
            return True
    
    # -- None
    if expected is None: # note: this means the expected was 'None', not just omitted (translated to Anything)
        if result is None:
            return True
        else:
            return FalseStr('expected None, but got: %r (%s)' % (result, type(result).__name__))
        return result is None
    
    # -- number
    if isinstance(expected, (int, long, float)):
        if not isinstance(result, (int, long, float)):
            return FalseStr('expected number %s but got %s (%s)' % (expected, result, type(result).__name__))
        if abs(expected - result) <= options['precision']:
            return True
        else:
            if options['precision']:
                return FalseStr('value %r was not within %r of %r' % (result, options['precision'], expected))
            else:
                return FalseStr('value %r was not equal to %r' % (result, expected))
    
    # -- string/unicode
    if isinstance(expected, (str, unicode)):
        if result == expected:
            return True
        else:
            return FalseStr('value %r was not the expected %s' % (result, expected))
    
    # -- dict
    if isinstance(expected, dict):
        if not isinstance(result, dict):
            return FalseStr('expected dict, got %s (%s)' % (result, type(result).__name__))
        
        # - keys
        expectedKeys = set(expected.keys())
        resultKeys = set(result.keys())
        if options['partial']:
            if not expectedKeys.issubset(resultKeys):
                return FalseStr('unmatched keys: %s' % expectedKeys.difference(resultKeys))
        else:
            if not expectedKeys == resultKeys:
                return FalseStr('unmatched keys from either side: %s' % expectedKeys.symmetric_difference(resultKeys))
        
        # - values
        for key, value in expected.items():
            compareResult = compare(value, result[key], options=options)
            if not compareResult:
                return compareResult
        
        # - all found
        return True
    
    # -- list/tuple/array
    if hasattr(expected, '__iter__'):
        if not hasattr(result, '__iter__'):
            return FalseStr('expected iterable, got %s (%s)' % (result, type(result).__name__))
        
        # - ordered
        if options['ordered']:
            haystack = result
            if not hasattr(haystack, 'next'):
                haystack = iter(result)
            for needle in expected:
                try:
                    while True:
                        straw = next(haystack)
                        if compare(needle, straw, options=options):
                            break
                        elif not options['partial']:
                            return FalseStr('got an unexpected item: %r while looking for %r' % (straw, needle))
                except StopIteration:
                    return FalseStr('ran out of results before finding: %r' % needle)
            if not options['partial']:
                try:
                    straw = next(haystack)
                    return FalseStr('found at least one extra result: %r' % straw)
                except StopIteration: pass
        
        # - unordered
        else:
            haystack = list(result)
            for needle in expected:
                for straw in haystack:
                    if compare(needle, straw, options=options):
                        break
                else:
                    return FalseStr('missing expected item: %r' % needle)
                haystack.remove(straw)
            if haystack and not options['partial']:
                return FalseStr('extra items returned: %r' % haystack)
        
        # - reutrn sucess
        return True
    
    # -- exception
    if isinstance(expected, (Err, Exception)):
        
        # - type
        
        if isinstance(expected, Err):
            if not isinstance(result, expected.err_type):
                return FalseStr('expected error type %s, got %r (%s)' % (expected.err_type, result, type(result).__name__))
        elif not isinstance(result, type(expected)):
            return FalseStr('expected error type %s, got %r (%s)' % (type(expected).__name__, result, type(result).__name__))
        
        # - message
        
        if expected.message:
            
            # strip details from output message
            resultMessage = None
            with warnings.catch_warnings():
                warnings.simplefilter('ignore')
                if hasattr(result, 'message'):
                    resultMessage = str(result.message)
                else:
                    resultMessage = str(result)
            resultMessage = re.sub(Err.exceptionRegex, '\g<message>:', resultMessage)
            compareResult = compare(expected.message, resultMessage, options=options)
            if not compareResult:
                return compareResult
        
        # - frames -- ToDo: implement this
        
        return True
    
    # -- Regex/UUID
    if isinstance(expected, (Regex, re._pattern_type)):
        match = expected.match(result)
        if match:
            return True
        else:
            return FalseStr('expected match for %s, but got: %s' % (expected, result))
    
    # -- type
    if not isinstance(expected, type(result)) and (type(expected) != type(object) or not issubclass(expected, type(result))): # reversed to so we can handle subclasses
        return FalseStr('expected type %s, got: %r (%s)' % (expected, result, type(result).__name__))
    
    # -- other
    if result != expected:
        return FalseStr('expected %r but got %r (%s)' % (expected, result, type(result).__name__))
    else:
        return True

# -- Curried output test functions --

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
            print_failure('--define--', expr, 'Exception while processing define', str(e))
    
    def run(self, src, expected, name, runopts, testopts):
        global passed_count
        
        print_debug('Test: %s' % name)
        
        if runopts:
            runopts["profile"] = True
        else:
            runopts = {"profile": True}
        
        compareOptions = {}
        if 'precision' in testopts:
            compareOptions['precision'] = float(testopts['precision']) # errors will bubble up
        
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
            exp_val = Anything()
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
        compareResult = compare(exp_val, result, options=compareOptions)
        if compareResult:
            passed_count += 1
        else:
            print_failure(name, src, exp_val, result, message=compareResult)

if __name__ == '__main__':
    driver = PyTestDriver()

# Emitted test code will consist of calls to these functions

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

def bag(expected, ordered=False, partial=None):
    options = {'ordered':ordered}
    if partial is not None:
        options['partial'] = partial
    if isinstance(expected, OptionsBox):
        newoptions = copy.copy(expected.options)
        newoptions.update(options)
        options = newoptions
        expected = expected.value
        
    assert isinstance(expected, (list, tuple)), \
        'bag can only work on lists, or tuples, got: %s (%r)' % (type(expected).__name__, expected)
    
    return OptionsBox(expected, options)

def partial(expected, ordered=False, partial=True):
    options = {'ordered':ordered, 'partial':partial}
    if isinstance(expected, OptionsBox):
        newoptions = copy.copy(expected.options)
        newoptions.update(options)
        options = newoptions
        expected = expected.value
    
    assert isinstance(expected, (dict, list, tuple)), \
        'partial can only work on dicts, lists, or tuples, got: %s (%r)' % (type(expected).__name__, expected)
    return OptionsBox(expected, options)

def fetch(cursor, limit=None, timeout=0.2):
    '''Pull items from a cursor'''

    # -- input validation

    if limit is not None:
        try:
            limit = int(limit)
            assert limit > 0
        except Exception:
            raise ValueError('invalid value for limit: %r' % limit)

    if timeout not in (None, 0):
        try:
            timeout = float(timeout)
            assert timeout > 0
        except Exception:
            raise ValueError('invalid value for timeout: %r' % timeout)

    # -- collect results

    result = []
    deadline = time.time() + timeout if timeout else None
    while (deadline is None) or (time.time() < deadline):
        try:
            if deadline:
                result.append(cursor.next(wait=deadline - time.time()))
            else:
                result.append(cursor.next())
            if limit and len(result) >= limit:
                break
        except r.ReqlTimeoutError:
            if limit is not None: # no error unless we get less than we expect
                result.append(r.ReqlTimeoutError())
            break
    else:
        result.append(r.ReqlTimeoutError())
    return result

def wait(seconds):
    '''Sleep for some seconds'''
    time.sleep(seconds)

def err(err_type, message=None, frames=None):
    return Err(err_type, message, frames)

def err_regex(err_type, message=None, frames=None):
    return Err(err_type, re.compile(message), frames)

def arrlen(length, thing=Anything()):
    return [thing] * length

def uuid():
    return Uuid()

def regex(value):
    return Regex(value)

def int_cmp(expected_value):
    if not isinstance(expected_value, (int, long)):
        raise ValueError('value must be of type `int` or `long` but got: %r (%s)' % (expected_value, type(expected_value).__name__))
    return OptionsBox(expected_value, {'explicit_type': (int, long)})

def float_cmp(expected_value):
    if not isinstance(expected_value, float):
        raise ValueError('value must be of type `float` but got: %r (%s)' % (expected_value, type(expected_value).__name__))
    return OptionsBox(expected_value, {'explicit_type': float})

def the_end():
    if failure_count > 0:
        sys.exit("Failed %d tests, passed %d" % (failure_count, passed_count))
    else:
        print("Passed all %d tests" % passed_count)

false = False
true = True
