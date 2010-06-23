#!/usr/bin/python

import sys
import subprocess
from multiprocessing import Pool
import memcache
from random import shuffle

NUM_INTS=100
NUM_THREADS=2
HOST="localhost"
PORT="11211"

# TODO: when we add more integration tests, the act of starting a
# RethinkDB process should be handled by a common external script.

def rethinkdb_insert(ints):
    mc = memcache.Client([HOST + ":" + PORT], debug=0)
    for i in ints:
	print "Inserting %d" % i
        mc.set(str(i), str(i))
    mc.disconnect_all()

def rethinkdb_verify():
    mc = memcache.Client([HOST + ":" + PORT], debug=0)
    for i in xrange(0, NUM_INTS):
        val = mc.get(str(i))
        if str(i) != val:
            print "Error, incorrent value in the database! (%d=>%s)" % (i, val)
            sys.exit(-1)
    mc.disconnect_all()

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
    shuffle(ints)
    
    # Invoke processes to insert them into the server concurrently
    # (Pool automagically waits for the processes to end)
    print "Inserting numbers"
    p = Pool(NUM_THREADS)
    p.map(rethinkdb_insert, split_list(ints, NUM_THREADS))
    p.close()
    p.join()

    # Verify that all integers have successfully been inserted
    print "Verifying"
    rethinkdb_verify()
    
    # Kill RethinkDB process
    # TODO: send the shutdown command
    print "Shutting down server"
    #rdb.stdin.writeLine("shutdown")
    #rdb.wait()

if __name__ == '__main__':
    sys.exit(main(sys.argv))
