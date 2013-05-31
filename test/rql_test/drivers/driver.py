from sys import path
import sys
import pdb
import collections
import types
import re
path.insert(0, '.')
from test_util import shard_table
path.insert(0, "../../drivers/python")

from os import environ
import rethinkdb as r

# JSPORT = int(sys.argv[1])
CPPPORT = int(sys.argv[2])
CLUSTER_PORT = int(sys.argv[3])
BUILD = sys.argv[4]

# -- utilities --

failure_count = 0

def print_test_failure(test_name, test_src, message):
    global failure_count
    failure_count = failure_count + 1
    print ''
    print "TEST FAILURE: %s" % test_name.encode('utf-8')
    print "TEST BODY: %s" % test_src.encode('utf-8')
    print message
    print ''

class Lst:
    def __init__(self, lst):
        self.lst = lst

    def __eq__(self, other):
        if not hasattr(other, '__iter__'):
            return False

        i = 0
        for row in other:
            if i >= len(self.lst) or (self.lst[i] != row):
                return False
            i += 1

        if i != len(self.lst):
            return False

        return True

    def __repr__(self):
        return repr(self.lst)

class Bag(Lst):
    def __init__(self, lst):
        self.lst = sorted(lst)

    def __eq__(self, other):
        if not hasattr(other, '__iter__'):
            return False

        other = sorted(other)

        if len(self.lst) != len(other):
            return False

        for i in xrange(len(self.lst)):
            if not self.lst[i] == other[i]:
                return False

        return True

class Dct:
    def __init__(self, dct):
        self.dct = dct

    def __eq__(self, other):
        if not isinstance(other, types.DictType):
            return False

        for key in self.dct.keys():
            if not key in other.keys():
                return False
            val = other[key]
            if isinstance(val, str) or isinstance(val, unicode):
                # Remove additional error info that creeps in in debug mode
                val = re.sub("\nFailed assertion:.*", "", val, flags=re.M|re.S)
            other[key] = val
            if not self.dct[key] == other[key]:
                return False
        return True

    def __repr__(self):
        return repr(self.dct)

class Err:
    def __init__(self, err_type=None, err_msg=None, err_frames=None, regex=False):
        self.etyp = err_type
        self.emsg = err_msg
        self.frames = None #err_frames # TODO: test frames
        self.regex = regex

    def __eq__(self, other):
        if not isinstance(other, Exception):
            return False

        if self.etyp and self.etyp != other.__class__.__name__:
            return False

        if self.regex:
            return re.match(self.emsg, other.message)

        else:

            # Strip "offending object" from the error message
            other.message = re.sub(":\n.*", ".", other.message, flags=re.M|re.S)
            other.message = re.sub("\nFailed assertion:.*", "", other.message, flags=re.M|re.S)

            if self.emsg and self.emsg != other.message:
                return False

            if self.frames and self.frames != other.frames:
                return False

            return True

    def __repr__(self):
        return "%s(%s\"%s\")" % (self.etyp, self.regex and '~' or '', repr(self.emsg) or '')


class Arr:
    def __init__(self, length, thing=None):
        self.length = length
        self.thing = thing

    def __eq__(self, arr):
        if not isinstance(arr, list):
            return False

        if not self.length == len(arr):
            return False

        if self.thing is None:
            return True

        return all([v == self.thing for v in arr])

    def __repr__(self):
        return "arr(%d, %s)" % (self.length, repr(self.thing))

class Uuid:
    def __eq__(self, thing):
        if not isinstance(thing, types.StringTypes):
            return False
        return re.match("[a-z0-9]{8}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{12}", thing) != None

    def __repr__(self):
        return "uuid()"

class Int:
    def __init__(self, i):
        self.i = i

    def __eq__(self, thing):
        return isinstance(thing, int) and (self.i == thing)

class Float:
    def __init__(self, f):
        self.f = f

    def __eq__(self, thing):
        return isinstance(thing, float) and (self.f == thing)

# -- Curried output test functions --

def eq(exp):
    if exp == ():
        return lambda x: True

    if isinstance(exp, list):
        exp = Lst(exp)
    elif isinstance(exp, dict):
        exp = Dct(exp)

    def sub(val):
        if not (val == exp):
            return False
        else:
            return True
    return sub

class PyTestDriver:

    # Set up connections to each database server
    def connect(self):
        #print 'Connecting to JS server on port ' + str(JSPORT)
        #self.js_conn = r.connect(host='localhost', port=JSPORT)

        print 'Connecting to CPP server on port ' + str(CPPPORT)
        print ''
        self.cpp_conn = r.connect(host='localhost', port=CPPPORT)
        r.db_create('test').run(self.cpp_conn)
        self.scope = {}

    def define(self, expr):
        exec(expr, globals(), self.scope)

    def run(self, src, expected, name, runopts):

        # Try to build the expected result
        if expected:
            exp_val = eval(expected, dict(globals().items() + self.scope.items()))
        else:
            # This test might not have come with an expected result, we'll just ensure it doesn't fail
            exp_val = ()

        # Try to build the test
        try:
            query = eval(src, dict(globals().items() + self.scope.items()))
        except Exception as err:
            if not isinstance(exp_val, Err):
                print_test_failure(name, src, "Error eval'ing test src:\n\t%s" % repr(err))
            elif not eq(exp_val)(err):
                print_test_failure(name, src,
                    "Error eval'ing test src not equal to expected err:\n\tERROR: %s\n\tEXPECTED: %s" %
                        (repr(err), repr(exp_val))
                )

            return # Can't continue with this test if there is no test query

        # Try actually running the test
        try:
            cppres = query.run(self.cpp_conn, **runopts)

            # And comparing the expected result
            if not eq(exp_val)(cppres):
                print_test_failure(name, src,
                    "CPP result is not equal to expected result:\n\tVALUE: %s\n\tEXPECTED: %s" %
                        (repr(cppres), repr(exp_val))
                )

        except Exception as err:
            if not isinstance(exp_val, Err):
                print_test_failure(name, src, "Error running test on CPP server:\n\t%s" % repr(err))
            elif not eq(exp_val)(err):
                print_test_failure(name, src,
                    "Error running test on CPP server not equal to expected err:\n\tERROR: %s\n\tEXPECTED: %s" %
                        (repr(err), repr(exp_val))
                )

driver = PyTestDriver()
driver.connect()

# Emitted test code will consist of calls to this function
def test(query, expected, name, runopts={}):
    if expected == '':
        expected = None
    driver.run(query, expected, name, runopts)

# Emitted test code can call this function to define variables
def define(expr):
    driver.define(expr)

# Emitted test code can call this function to set bag equality on a list
def bag(lst):
    return Bag(lst)

# Emitted test code can call this function to indicate expected error output
def err(err_type, err_msg=None, frames=None):
    return Err(err_type, err_msg, frames)

def err_regex(err_type, err_msg=None, frames=None):
    return Err(err_type, err_msg, frames, True)

def arrlen(length, thing=None):
    return Arr(length, thing)

def uuid():
    return Uuid()

def shard(table_name):
    shard_table(CLUSTER_PORT, BUILD, table_name)

def int_cmp(i):
    return Int(i)

def float_cmp(f):
    return Float(f)

def the_end():
    global failure_count
    if failure_count > 0:
        sys.exit("Failed %d tests" % failure_count)
