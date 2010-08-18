#!/usr/bin/python

import rethinkdb_memcache as memcache
from random import shuffle, randint
from time import sleep
import os

NUM_INTS=1600
NUM_THREADS=1

def rethinkdb_cas(mc, ints, clone):
    for i in ints:
        print "Inserting %d" % i
        if (0 == mc.set(str(i), str(i))):
            raise ValueError("Insert of %d failed" % i)
            
        print "Getting %d" % i
        value, _, cas_id = mc.explicit_gets(str(i))
        if (value != str(i)):
            raise ValueError("get failed, should be %d=>%d, was %s" % (i, i, value))
            
        print "'cas'-ing %d" % i
        if (0 == mc.explicit_cas(str(i), str(i+1), cas_id)):
            raise ValueError("cas of %d failed" % i)
            
        print "Verifying cas %d" % i
        value, _, cas_id = mc.explicit_gets(str(i))
        if (value != str(i+1)):
            raise ValueError("get for cas failed, should be %d=>%d, was %s" % (i, i+1, value))
            
        print "Modifying %d again" % i
        if (0 == mc.set(str(i), str(i+10))):
            raise ValueError("Modify of %d failed" % i)
            
        print "'cas'-ing %d again" % i
        if (0 != mc.explicit_cas(str(i), str(i+20), cas_id)):
            raise ValueError("cas of %d should have failed, item has been modified" % i)


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

    print "Checking cas on numbers"
    rethinkdb_cas(mc, ints, clone)
    
    print "Done"


from test_common import RethinkDBTester
retest_release = RethinkDBTester(test_against_server_at, "release", timeout = 20)
retest_valgrind = RethinkDBTester(test_against_server_at, "debug", valgrind=True, timeout = 60)

if __name__ == '__main__':
    test_against_server_at(int(os.environ.get("RUN_PORT", "11211")))
