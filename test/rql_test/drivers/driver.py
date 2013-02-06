from sys import path
import types
import pdb
import collections
path.append("/home/wmrowan/rethinkdb/drivers/python2")

from os import environ
import rethinkdb as r 

JSPORT = 28017
CPPPORT = 28016

# -- utilities --

def eq_test(one, two):
    if isinstance(one, list):

        if not isinstance(two, list):
            return False

        if len(one) != len(two):
            return False
        
        for i in xrange(len(one)):
            if not eq_test(one[i], two[i]):
                return False

        return True

    elif isinstance(one, dict):
        
        for key in one.keys():
            if not key in two.keys():
                return False
            if not eq_test(one[key], two[key]):
                return False

        return True

    else:
        
        # Primitive comparison
        return (one == two)

# -- Curried output test functions --

def eq(exp):
    def sub(val):
        if not eq_test(val, exp):
            print "Equality comparison failed"
            print "Value:", repr(val), "Expected:", repr(exp)
    return sub

class PyTestDriver:

    # Set up connections to each database server
    def connect(self):
        self.js_conn = r.connect(host='localhost', port=JSPORT)
        self.cpp_conn = r.connect(host='localhost', port=CPPPORT)

    def run(self, query, expected):
        try:
            query = eval(query)
        except Exception as err:
            raise Exception("Query construction error")

        try:
            cppres = query.run(self.cpp_conn)
        except Exception as err:
            cppres = err

        try:
            jsres = query.run(self.js_conn)
        except Exception as err:
            jsres = err

        exp_fun = eval(expected)
        if not isinstance(exp_fun, types.FunctionType):
            exp_fun = eq(exp_fun)

        exp_fun(cppres)
        exp_fun(jsres)

driver = PyTestDriver()
driver.connect()

# Emitted test code will consist of calls to this function
def test(query, expected):
    driver.run(query, expected)

