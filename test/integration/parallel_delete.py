#!/usr/bin/python

from multiprocessing import Pool, Queue, Process
import memcache
from random import shuffle


NUM_INTS=8000
NUM_THREADS=32

def rethinkdb_insert((port, ints)):
    mc = memcache.Client(["localhost:%d" % port])
    for i in ints:
        print "Inserting %d" % i
        ok = mc.set(str(i), str(i))
        if ok == 0: raise ValueError("Cannot insert %d" % i)
    mc.disconnect_all()

def rethinkdb_delete((port, ints)):
    mc = memcache.Client(["localhost:%d" % port])
    for i in ints:
        print "Deleting %d" % i
        ok = mc.delete(str(i))
        if ok == 0: raise ValueError("Cannot delete %d" % i)
    mc.disconnect_all()

def rethinkdb_verify(port):
    mc = memcache.Client(["localhost:%d" % port])
    for i in xrange(0, NUM_INTS):
        print "Verifying %d" % i
        val = mc.get(str(i))
        if str(i) != val:
            raise ValueError("Error, incorrent value in the database! (%d=>%s)" % (i, val))
    mc.disconnect_all()

def rethinkdb_verify_empty((port, in_ints, out_ints)):
    mc = memcache.Client(["localhost:%d" % port])
    for i in out_ints:
        print "Verifying deletion of %d" % i
        if mc.get(str(i)) == 0:
            raise ValueError("Error, value %d is in the database when it shouldn't be" % i)
    for i in in_ints:
        print "Verifying %d" % i
        val = mc.get(str(i))
        if str(i) != val:
            raise ValueError("Error, incorrent value in the database! (%d=>%s)" % (i, val))
    mc.disconnect_all()

def test_against_server_at(port):
    
    def split_list(alist, parts):
        length = len(alist)
        return [alist[i * length // parts: (i + 1) * length // parts]
                for i in range(parts)]
    
    # Create a list of integers we'll be inserting
    ints = range(0, NUM_INTS)
    ints2 = range(0, NUM_INTS)
    
    print "Starting inserters"
    lists = split_list(ints, NUM_THREADS)
    p_inserters = Pool(NUM_THREADS)
    inserter_results = p_inserters.map_async(rethinkdb_insert, [(port, list) for list in lists])
    p_inserters.close()
    inserter_results.get()
    
    print "Starting deleters"
    firstints2 = ints2[0 : NUM_INTS / 2]
    secondints2 = ints2[NUM_INTS / 2 : NUM_INTS]
    lists = split_list(firstints2, NUM_THREADS)
    p_deleters = Pool(NUM_THREADS)
    deleter_results = p_deleters.map_async(rethinkdb_delete, [(port, list) for list in lists])
    p_deleters.close()
    deleter_results.get()
    
    print "Verifying"
    rethinkdb_verify_empty((port, secondints2, firstints2))
    
    print "Done"

from test_common import RethinkDBTester
retest_release = RethinkDBTester(test_against_server_at, "release")
retest_valgrind = RethinkDBTester(test_against_server_at, "debug", valgrind=True)

if __name__ == '__main__':
    test_against_server_at(int(os.environ.get("RUN_PORT", "11211")))