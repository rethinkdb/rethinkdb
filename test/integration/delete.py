#!/usr/bin/python

import sys
import subprocess
#from multiprocessing import Pool, Queue, Process
import memcache
from random import shuffle
from time import sleep

NUM_INTS=8
NUM_THREADS=1
HOST="localhost"
PORT="11213"

# TODO: when we add more integration tests, the act of starting a
# RethinkDB process should be handled by a common external script.

def rethinkdb_insert(ints):
    mc = memcache.Client([HOST + ":" + PORT], debug=0)
    for i in ints:
        print "Inserting %d" % i
        if (0 == mc.set(str(i), str(i))):
            print "Insert failed"
            sys.exit(-1)
    sleep(1)
    mc.disconnect_all()

def rethinkdb_delete(ints):
    mc = memcache.Client([HOST + ":" + PORT], debug=0)
    if(0 == mc.servers[0].connect()):
        print "Failed to connect"
    for i in ints:
        print "Deleting %d" % i
        if (0 == mc.delete(str(i))):
            print "Delete failed"
            sys.exit(-1)
    sleep(1)
#mc.disconnect_all()

def rethinkdb_verify():
    mc = memcache.Client([HOST + ":" + PORT], debug=0)
    for i in xrange(0, NUM_INTS):
        val = mc.get(str(i))
        if str(i) != val:
            print "Error, incorrent value in the database! (%d=>%s)" % (i, val)
            sys.exit(-1)
    sleep(1)
    mc.disconnect_all()

def rethinkdb_verify_empty(in_ints, out_ints):
    mc = memcache.Client([HOST + ":" + PORT], debug=0)
    if(0 == mc.servers[0].connect()):
        print "Failed to connect"
    for i in out_ints:
        print "Get(", i, ")"
        val = mc.get(str(i))
        print i, "=>", val
        if (mc.get(str(i))):
            print "Error, value %d is in the database when it shouldn't be" % i
            sys.exit(-1)
    for i in in_ints:
        print "Get(", i, ")"
        val = mc.get(str(i))
        print "%s => %s" % (str(i), val)
    sleep(1)
#mc.disconnect_all()

def split_list(alist, parts):
    length = len(alist)
    return [alist[i * length // parts: (i + 1) * length // parts]
            for i in range(parts)]

def main(argv):
    # Start rethinkdb process
    #rdb = subprocess.Popen(["../../src/rethinkdb"],
    #                       stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    
    # Create a list of integers we'll be inserting
    print "Shuffling numbers"
    ints = range(0, NUM_INTS)
#shuffle(ints)
    ints2 = range(0, NUM_INTS)
#shuffle(ints2)
    
    # Invoke processes to insert them into the server concurrently
    # (Pool automagically waits for the processes to end)

    print "Inserting numbers"
    rethinkdb_insert(ints)

    # Verify that all integers have successfully been inserted
    print "Verifying"
    rethinkdb_verify()

    print "Deleting numbers"
    firstints2 = ints2[0:NUM_INTS / 2]
    secondints2 = ints2[NUM_INTS / 2: NUM_INTS]
    rethinkdb_delete(firstints2)

    print "Verifying"
    rethinkdb_verify_empty(secondints2, firstints2)
    
    # Kill RethinkDB process
    # TODO: send the shutdown command
    print "Shutting down server"
    #rdb.stdin.writeLine("shutdown")
    #rdb.wait()

if __name__ == '__main__':
    sys.exit(main(sys.argv))
