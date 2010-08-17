#!/usr/bin/python

import memcache
from random import shuffle, randint
from time import sleep
import os

def rethinkdb_incr(mc):
    if mc.set(str(1),str(1)) == 0:
        raise ValueError, "Set failed"
    mc.incr(str(1),10)
    if mc.get(str(1)) != str(11):
        raise ValueError, "simple increment fails, should have been 11, was", mc.get(str(1))

    # TODO: Figure out a way to test negative increments and incrementing by a very large value.
    # memcache doesn't allow either.


def rethinkdb_decr(mc):
    if mc.set(str(1),str(50)) == 0:
        raise ValueError, "Set failed"
    mc.decr(str(1),10)
    if mc.get(str(1)) != str(40):
        raise ValueError, "simple decrement fails, should have been 40, was", mc.get(str(1))

    # TODO: Figure out a way to test negative decrements and decrementing by a very large value.
    # memcache doesn't allow either.

def test_against_server_at(port):

    mc = memcache.Client(["localhost:%d" % port])
    print "Testing increment"
    rethinkdb_incr(mc)

    
    print "Done"

from test_common import RethinkDBTester
retest_release = RethinkDBTester(test_against_server_at, "release", timeout = 20)
retest_valgrind = RethinkDBTester(test_against_server_at, "debug", timeout = 60)

if __name__ == '__main__':
    test_against_server_at(int(os.environ.get("RUN_PORT", "11211")))
