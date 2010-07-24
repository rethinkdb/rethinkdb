#!/usr/bin/python

from multiprocessing import Pool, Queue, Process
import memcache
from random import shuffle, randint
import random, string, os

NUM_INSERTS=320
NUM_THREADS=16

def rethinkdb_insert((port, pairs)):
    mc = memcache.Client(["localhost:%d" % port])
    for pair in pairs:
        print "Inserting %s : %s" % pair
        if (0 == mc.set(pair[0], pair[1])):
            raise ValueError("Inserting %s : %s failed" % pair)
    mc.disconnect_all()

def rethinkdb_verify((port, pairs)):
    mc = memcache.Client(["localhost:%d" % port])
    for pair in pairs:
        print "Verifying %s : %s" % pair
        val = mc.get(pair[0])
        if pair[1] != val:
            raise ValueError("Error, incorrent value in the database! (%s=>%s)" % (pair[0], val))
    mc.disconnect_all()

def split_list(alist, parts):
    length = len(alist)
    return [alist[i * length // parts: (i + 1) * length // parts]
            for i in range(parts)]

def rethinkdb_delete(mc, keys, clone):
    for key in keys:
        print "Deleting %s" % key
        if (0 == mc.delete(key)):
            raise ValueError("Delete failed")
        del clone[key]
        if (randint(1,100)==1):
            print "Verifying"
            rethinkdb_cloned_verify(mc, keys, clone)

def rethinkdb_cloned_verify(mc, keys, clone):
    for key in keys:
        print "Verifying %s" % key
        stored_value = mc.get(key)
        clone_value = clone.get(key)
        if clone_value != stored_value:
            raise ValueError("Error, incorrent value in the database! (%s=>%s, should be %s)" % \
                (key, stored_value, clone_value))

def test_against_server_at(port):
    
    print "Creating Pairs"
    keys = set()
    for i in xrange(NUM_INSERTS):
        key_num = random.randint(1, 250)
        key_char = random.choice(string.letters + string.digits)
        keys.add(key_num*key_char)

    pairs = map(lambda key: (key,random.choice(string.letters + string.digits)*random.randint(1, 250)), keys)
    
    lists = split_list(pairs, NUM_THREADS)

    print "Inserting numbers"
    p_inserters = Pool(NUM_THREADS)
    inserter_results = p_inserters.map_async(rethinkdb_insert, [(port, l) for l in lists])
    p_inserters.close()
    inserter_results.get()

    print "Verifying"
    rethinkdb_verify((port,pairs))

    mc = memcache.Client(["localhost:%d" % port])

    rethinkdb_delete(mc, list(keys), dict(pairs))
    
    print "Done"

from test_common import RethinkDBTester
retest_release = RethinkDBTester(test_against_server_at, "release")
retest_valgrind = RethinkDBTester(test_against_server_at, "debug", valgrind=True)

if __name__ == '__main__':
    test_against_server_at(int(os.environ.get("RUN_PORT", "11211")))