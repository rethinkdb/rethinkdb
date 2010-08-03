#!/usr/bin/python
""" 
    We want to make sure that the data doesn't get corrupted if the server is abruptly killed.

"""


import pylibmc as memcache
from random import shuffle, randint
from time import sleep
import os

NUM_INTS = 300
NUM_THREADS = 1
bin = False
ints = range(0, NUM_INTS)
clone = {}

def rethinkdb_insert(mc, ints, clone, mutual_list):
    for i in ints:
        print "Inserting %d" % i
        mutual_list.append(i)
        if (0 == mc.set(str(i), str(i))):
            raise ValueError("Insert of %d failed" % i)
        clone[str(i)] = str(i)
        
def rethinkdb_verify(mc, ints, clone, mutual_list):
    for i in ints:
        print "Verifying existence of %d" % i
        mutual_list.append(i)
        key = str(i)
        stored_value = mc.get(key)
        actual_value = key
        print "(%s=>%s, should be %s)" % (key, stored_value, actual_value)
        if actual_value != stored_value:
            raise ValueError("Error, incorrect value in the database! (%s=>%s, should be %s). %d keys correctly stored." % \
                (key, stored_value, actual_value, i-1))

def test_insert(port, mutual_list):
    mc = memcache.Client(["localhost:%d" % port], binary = bin)
    print "Inserting numbers"
    rethinkdb_insert(mc, ints, clone, mutual_list)
    print "Done"

def test_verify(port, mutual_list):
    mc = memcache.Client(["localhost:%d" % port], binary = bin)
    print "Verifying numbers"
    rethinkdb_verify(mc, ints, clone, mutual_list)    
    print "Done"

from test_common import RethinkDBCorruptionTester
retest_release = RethinkDBCorruptionTester(test_insert, test_verify, "release", timeout = 10)
retest_valgrind = RethinkDBCorruptionTester(test_insert, test_verify, "debug", timeout = 10)

if __name__ == '__main__':
    print "The corruption test involves shutting down and restarting a server, so it can't be run this way."