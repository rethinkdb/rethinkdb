#!/usr/bin/python

import memcache
from random import shuffle, randint
import random, string, os, sys

LOG_FILE="log.txt"
NUM_INSERTS=8000
log = sys.stdout

def rethinkdb_insert(port, pairs):
    mc = memcache.Client(["localhost:%d" % port], debug=0)
    for pair in pairs:
        log.write("set %s %s\n" % pair)
        if (0 == mc.set(pair[0], pair[1])):
            raise Exception("Insertion failed.")
    mc.disconnect_all()

def rethinkdb_verify(port, pairs):
    log.write("verify\n")
    error = False
    mc = memcache.Client(["localhost:%d" % port], debug=0)
    for pair in pairs:
        val = mc.get(pair[0])
        if pair[1] != val:
            print "Error, incorrent value in the database! (%s=>%s)" % (pair[0], val)
            error = True
    mc.disconnect_all()
    if error:
        raise Exception("Verification failed.")

def rethinkdb_delete(mc, keys, clone):
    for key in keys:
        log.write("delete %s\n" % key)
        if (0 == mc.delete(key)):
            raise Exception("Deletion failed.")
        del clone[key]
        if (randint(1,1)==1000):
            rethinkdb_cloned_verify(mc, keys, clone)

def rethinkdb_cloned_verify(mc, keys, clone):
    log.write("verify\n")
    error = False
    for key in keys:
        stored_value = mc.get(key)
        clone_value = clone.get(key)
        if clone_value != stored_value:
            print "Error, incorrent value in the database! (%s=>%s, should be %s)" % (key, stored_value, clone_value)
            error = True
    if error:
        raise Exception("Verification failed.")

def test_against_server_at(port):
    
    keys = set()
    for i in xrange(NUM_INSERTS):
        key_num = random.randint(1, 250)
        key_char = random.choice(string.letters + string.digits)
        
        keys.add(key_num*key_char)
    
    pairs = map(lambda key: (key,random.choice(string.letters + string.digits)*random.randint(1, 250)), keys)
    
    rethinkdb_insert(port, pairs)
    
    rethinkdb_verify(port, pairs)
    
    mc = memcache.Client(["localhost:%d" % port], debug=0)
    rethinkdb_delete(mc, list(keys), dict(pairs))
    mc.disconnect_all()

from test_common import RethinkDBTester
retest_release = RethinkDBTester(test_against_server_at, "release", timeout = 20)
retest_valgrind = RethinkDBTester(test_against_server_at, "debug", valgrind=True, timeout = 60)

if __name__ == '__main__':
    log = open(LOG_FILE, 'w')
    test_against_server_at(int(os.environ.get("RUN_PORT", "11211")))
    log.close()
