from sys import path
path.append("/home/wmrowan/rethinkdb/drivers/python2")

from os import environ
import rethinkdb as r 

JSPORT = 28017
CPPPORT = 28016

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

        expected = eval(expected)

        if cppres != jsres:
            print cppres, jsres
            raise Exception("Two results not equal")

        if cppres != expected:
            print cppres, expected
            raise Exception("Cpp res not expected")

        if jsres != expected:
            print jsres, expected
            raise Exception("Js res not expected")

driver = PyTestDriver()
driver.connect()

# Emitted test code will consist of calls to this function
def test(query, expected):
    driver.run(query, expected)

