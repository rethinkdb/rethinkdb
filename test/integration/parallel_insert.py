#!/usr/bin/python

from multiprocessing import Pool, Queue, Process
import memcache
from random import shuffle
import os

NUM_INTS=800000
NUM_THREADS=1
NUMSTR = "%d"

def rethinkdb_insert((port, ints)):
    mc = memcache.Client(["localhost:%d" % port])
    for i in ints:
        print "Inserting %d" % i
        if (0 == mc.set(str(i), NUMSTR % i)):
            raise ValueError("Cannot insert %d" % i)
    mc.disconnect_all()

def rethinkdb_verify(port):
    mc = memcache.Client(["localhost:%d" % port])
    for i in xrange(0, NUM_INTS):
        print "Checking %d" % i
        val = mc.get(str(i))
        if NUMSTR % i != val:
            raise ValueError("Error, incorrent value in the database! (%d=>%s)" % (i, val))
    mc.disconnect_all()

def test_against_server_at(port):
    
    # Create a list of integers we'll be inserting
    ints = range(0, NUM_INTS)
#shuffle(ints)
    
    def split_list(alist, parts):
        length = len(alist)
        return [alist[i * length // parts: (i + 1) * length // parts]
                for i in range(parts)]
    lists = split_list(ints, NUM_THREADS)

    print "Starting inserters"
    p_inserters = Pool(NUM_THREADS)
    inserter_results = p_inserters.map_async(rethinkdb_insert, [(port, list) for list in lists])
    p_inserters.close()
    inserter_results.get()

    print "Verifying"
    rethinkdb_verify(port)
    
    print "Done"

from test_common import RethinkDBTester
retest_release = RethinkDBTester(test_against_server_at, "release", timeout = 20)
retest_valgrind = RethinkDBTester(test_against_server_at, "debug", valgrind=True, timeout = 60)

if __name__ == '__main__':
    test_against_server_at(int(os.environ.get("RUN_PORT", "11211")))
