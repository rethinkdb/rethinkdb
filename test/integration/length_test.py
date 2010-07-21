#!/usr/bin/python

import os
import sys
import subprocess
from multiprocessing import Pool, Queue, Process
import memcache
from random import shuffle, randint
import random
import string

LOG_FILE="log.txt"
NUM_INSERTS=32000
HOST="localhost"
PORT=os.getenv("RUN_PORT", "11211")
log = open(LOG_FILE, 'w')

# TODO: when we add more integration tests, the act of starting a
# RethinkDB process should be handled by a common external script.

def rethinkdb_insert(pairs):
    mc = memcache.Client([HOST + ":" + PORT], debug=0)
    for pair in pairs:
        print "Inserting %s : %s" % pair
        log.write("set %s %s\n" % pair)
        if (0 == mc.set(pair[0], pair[1])):
            print "Insertion failed"
            sys.exit(-1)
    mc.disconnect_all()

def rethinkdb_verify(pairs):
    log.write("verify\n")
    error = False
    mc = memcache.Client([HOST + ":" + PORT], debug=0)
    for pair in pairs:
        val = mc.get(pair[0])
        if pair[1] != val:
            print "Error, incorrent value in the database! (%s=>%s)" % (pair[0], val)
            error = True
    mc.disconnect_all()
    if error:
        sys.exit(-1)

def split_list(alist, parts):
    length = len(alist)
    return [alist[i * length // parts: (i + 1) * length // parts]
            for i in range(parts)]

def rethinkdb_delete(mc, keys, clone):
    for key in keys:
        print "Deleting %s" % key
        log.write("delete %s\n" % key)
        if (0 == mc.delete(key)):
            print "Delete failed"
            sys.exit(-1)
        del clone[key]
        if (randint(1,1)==1000):
            print "Verifying"
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
        sys.exit(-1)

def main(argv):
    # Start rethinkdb process
    #rdb = subprocess.Popen(["../../src/rethinkdb"],
    #                       stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    
    # Create a list of integers we'll be inserting
    print "Creating Pairs"
    keys = set()
    for i in xrange(NUM_INSERTS):
        key_num = random.randint(1, 250)
        key_char = random.choice(string.letters + string.digits)

        keys.add(key_num*key_char)

    pairs = map(lambda key: (key,random.choice(string.letters + string.digits)*random.randint(1, 250)), keys)
    
    print "Inserting numbers"
    rethinkdb_insert(pairs)


    # Verify that all integers have successfully been inserted
    print "Verifying"
    rethinkdb_verify(pairs)

    mc = memcache.Client([HOST + ":" + PORT], debug=0)

    rethinkdb_delete(mc, list(keys), dict(pairs))
    
    log.close()
    
    # Kill RethinkDB process
    # TODO: send the shutdown command
    print "Shutting down server"
    #rdb.stdin.writeLine("shutdown")
    #rdb.wait()

if __name__ == '__main__':
    sys.exit(main(sys.argv))
