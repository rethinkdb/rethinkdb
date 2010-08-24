#!/usr/bin/python

import memcache
from random import shuffle, randint
from time import sleep
import os

NUM_INTS=20000
NUM_THREADS=1

def rethinkdb_insert(mc, ints, clone):
    for i in ints:
        print "Inserting %d" % i
        if (0 == mc.set(str(i), str(i))):
            raise ValueError("Insert of %d failed" % i)
        clone[str(i)] = str(i)

def rethinkdb_delete(mc, ints, clone):
    for i in ints:
        print "Deleting %d" % i
        if (0 == mc.delete(str(i))):
            raise ValueError("Delete of %d failed" % i)
        del clone[str(i)]
        if (randint(1,1000)==1):
            print "Verifying"
            rethinkdb_verify(mc, ints, clone)

def rethinkdb_verify(mc, ints, clone):
    for i in ints:
        key = str(i)
        stored_value = mc.get(key)
        clone_value = clone.get(key)
        if clone_value != stored_value:
            raise ValueError("Error, incorrent value in the database! (%s=>%s, should be %s)" % \
                (key, stored_value, clone_value))

def split_list(alist, parts):
    length = len(alist)
    return [alist[i * length // parts: (i + 1) * length // parts]
            for i in range(parts)]

def test_against_server_at(port):

    mc = memcache.Client(["localhost:%d" % port])
    clone = {}

    print "Shuffling numbers"
    ints = range(0, NUM_INTS)
    shuffle(ints)

    print "Inserting numbers"
    rethinkdb_insert(mc, ints, clone)

    print "Verifying"
    rethinkdb_verify(mc, ints, clone)

    print "Deleting numbers"
    rethinkdb_delete(mc, ints, clone)
    
    print "Done"

from test_common import RethinkDBTester
retest_release = RethinkDBTester(test_against_server_at, "release", timeout = 20)
retest_valgrind = RethinkDBTester(test_against_server_at, "debug", valgrind=True, timeout = 60)

if __name__ == '__main__':
    test_against_server_at(int(os.environ.get("RUN_PORT", "11211")))
