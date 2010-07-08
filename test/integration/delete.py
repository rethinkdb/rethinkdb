#!/usr/bin/python

import sys
import subprocess
#from multiprocessing import Pool, Queue, Process
import memcache
from random import shuffle, randint
from time import sleep

NUM_INTS=8000
NUM_THREADS=1
HOST="localhost"
PORT="11212"

# TODO: when we add more integration tests, the act of starting a
# RethinkDB process should be handled by a common external script.


def rethinkdb_insert(mc, ints, clone):
    for i in ints:
        print "Inserting %d" % i
        if (0 == mc.set(str(i), str(i))):
            print "Insert failed"
            sys.exit(-1)
        clone[str(i)] = str(i)

def rethinkdb_delete(mc, ints, clone):
    for i in ints:
        print "Deleting %d" % i
        if (0 == mc.delete(str(i))):
            print "Delete failed"
            sys.exit(-1)
        del clone[str(i)]
        if (randint(1,100)==1):
            print "Verifying"
            rethinkdb_verify(mc, ints, clone)

def rethinkdb_verify(mc, ints, clone):
    for i in ints:
        key = str(i)
        stored_value = mc.get(key)
        clone_value = clone.get(key)
        if clone_value != stored_value:
            print "Error, incorrent value in the database! (%s=>%s, should be %s)" % (key, stored_value, clone_value)
            sys.exit(-1)

def split_list(alist, parts):
    length = len(alist)
    return [alist[i * length // parts: (i + 1) * length // parts]
            for i in range(parts)]

def main(argv):
    mc = memcache.Client([HOST + ":" + PORT], debug=0)
    clone = {}
    # Create a list of integers we'll be inserting
    print "Shuffling numbers"
    ints = range(0, NUM_INTS)
    shuffle(ints)
    
    # Invoke processes to insert them into the server concurrently
    # (Pool automagically waits for the processes to end)

    print "Inserting numbers"
    rethinkdb_insert(mc, ints, clone)

#sys.exit(0)
    # Verify that all integers have successfully been inserted
    print "Verifying"
    rethinkdb_verify(mc, ints, clone)

    print "Deleting numbers"
    rethinkdb_delete(mc, ints, clone)

    # Kill RethinkDB process
    # TODO: send the shutdown command
    print "Shutting down server"
    #rdb.stdin.writeLine("shutdown")
    #rdb.wait()

if __name__ == '__main__':
    sys.exit(main(sys.argv))
