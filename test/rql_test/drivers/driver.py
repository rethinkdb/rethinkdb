from sys import path
import sys
import types
import pdb
import collections
path.append("../../drivers/python2")

from os import environ
import rethinkdb as r 

JSPORT = int(sys.argv[1])
CPPPORT = int(sys.argv[2])

# -- utilities --

class Lst:
    def __init__(self, lst):
        self.lst = lst

    def __eq__(self, other):
        if len(self.lst) != len(other):
            return False
        
        for i in xrange(len(self.lst)):
            if not (self.lst[i] == other[i]):
                return False

        return True

    def __repr__(self):
        return repr(self.lst)

class Bag(Lst):
    def __init__(self, lst):
        self.lst = sorted(lst)

    def __eq__(self, other):
        other = sorted(other)

        if len(self.lst) != len(other):
            return False
        
        for i in xrange(len(self.lst)):
            if not (self.lst[i] == other[i]):
                return False

        return True

class Dct:
    def __init__(self, dct):
        self.dct = dct
    
    def __eq__(self, other): 
        for key in self.dct.keys():
            if not key in other.keys():
                return False
            if not (self.dct[key] == other[key]):
                return False
        return True

    def __repr__(self):
        return repr(self.dct)

class Err:
    def __init__(self, err_type=None, err_msg=None, err_frames=None):
        self.etyp = err_type
        self.emsg = err_msg
        self.frames = err_frames

    def __eq__(self, other):
        if not isinstance(other, Exception):
            return False

        if self.etyp and self.etyp != other.__class__.__name__:
            return False

        if self.emsg and self.emsg != other.message:
            return False

        if self.frames and self.frames != other.frames:
            return False

        return True

    def __repr__(self):
        return "%s(%s)" % (self.etyp, repr(self.emsg) or '')

# -- Curried output test functions --

def eq(exp):
    if isinstance(exp, list):
        exp = Lst(exp)
    elif isinstance(exp, dict):
        exp = Dct(exp)

    def sub(val):
        if not (val == exp):
            print "Equality comparison failed"
            print "Value:", repr(val), "Expected:", repr(exp)
            return False
        else:
            return True
    return sub

class PyTestDriver:

    # Set up connections to each database server
    def connect(self):
        print 'Connecting to JS server on port ' + str(JSPORT)
        self.js_conn = r.connect(host='localhost', port=JSPORT)
        print 'Connecting to CPP server on port ' + str(CPPPORT)
        self.cpp_conn = r.connect(host='localhost', port=CPPPORT)
        self.scope = {}

    def define(self, expr):
        exec(expr, globals(), self.scope)

    def run(self, src, expected):

        # Try to build the expected result
        if expected:
            exp_fun = eval(expected, dict(globals().items() + self.scope.items()))
        else:
            # This test might not have come with an expected result, we'll just ensure it doesn't fail
            exp_fun = lambda v: True

        # If left off the comparison function is equality by default
        if not isinstance(exp_fun, types.FunctionType):
            exp_fun = eq(exp_fun)

        # Try to build the test
        try:
            query = eval(src, dict(globals().items() + self.scope.items()))
        except Exception as err:
            if not exp_fun(err):
                print "Python error on construction of query:", str(err), 'in:\n', src
                return # Can't continue with this test if there is no test query

        # Try actually running the test
        try:
            cppres = query.run(self.cpp_conn)

            # And comparing the expected result
            if not exp_fun(cppres):
                print " in CPP version of:", src

        except Exception as err:
            if not exp_fun(err):
                print "Error on running of query on CPP server:", str(err), 'in:\n', src

        try:
            jsres = query.run(self.js_conn)
            if not exp_fun(jsres):
                print " in JS version of:", src
        except Exception as err:
            if not exp_fun(err):
                print "Error on running of query on JS server:", str(err), 'in:\n', src

driver = PyTestDriver()
driver.connect()

# Emitted test code will consist of calls to this function
def test(query, expected, name):
    if expected == '':
        expected = None
    driver.run(query, expected)

# Emitted test code can call this function to define variables
def define(expr):
    driver.define(expr)

# Emitted test code can call this function to set bag equality on a list
def bag(lst):
    return Bag(lst)

# Emitted test code can call this function to indicate expected error output
def err(err_type, err_msg=None, frames=None):
    return Err(err_type, err_msg, frames)
