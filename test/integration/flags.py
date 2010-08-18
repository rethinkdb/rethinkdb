#!/usr/bin/python

import rethinkdb_memcache as memcache
from random import shuffle, randint
from time import sleep
import os

NUM_INTS=1600
NUM_THREADS=1

def rethinkdb_flags(mc, ints, clone):

    # flag values are:
    #   - 0 for strings
    #   - 2 for ints

    for i in ints:
        print "Inserting %d" % i
        if i % 2: val = str(i)
        else: val = i
            
        if (0 == mc.set(str(i), val)):
            raise ValueError("Insert of %d failed" % i)
            
        print "Getting %d" % i
        value, flags, _ = mc.explicit_gets(str(i))
        if (value != val):
            raise ValueError("get failed, should be %d=>%d, was %s" % (i, val, value))
        
        print "Checking flag for %d" % i
        if i % 2:
            if flags != 0:
                raise ValueError("flag set failed, should be %d, was %d" % (0, flags))
        else:        
            if flags != 2:
                raise ValueError("flag set failed, should be %d, was %d" % (2, flags))
        
        
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

    print "Checking flags on numbers"
    rethinkdb_flags(mc, ints, clone)
    
    
    print "Done"


from test_common import RethinkDBTester
retest_release = RethinkDBTester(test_against_server_at, "release", timeout = 20)
retest_valgrind = RethinkDBTester(test_against_server_at, "debug", valgrind=True, timeout = 60)

if __name__ == '__main__':
    test_against_server_at(int(os.environ.get("RUN_PORT", "11211")))
