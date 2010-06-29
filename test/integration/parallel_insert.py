#!/usr/bin/python

import sys
import subprocess
from multiprocessing import Pool, Queue, Process
import memcache
from random import shuffle

NUM_INTS=32000
NUM_THREADS=32
HOST="localhost"
PORT="11211"

# TODO: when we add more integration tests, the act of starting a
# RethinkDB process should be handled by a common external script.

def rethinkdb_insert(queue, ints):
    mc = memcache.Client([HOST + ":" + PORT], debug=0)
    for i in ints:
        print "Inserting %d" % i
        if (0 == mc.set(str(i), str(i))):
            queue.put(-1)
            return
    mc.disconnect_all()
    queue.put(0)

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
    lists = split_list(ints, NUM_THREADS)

    print "Inserting numbers"
    queue = Queue()
    procs = []
    for i in xrange(0, NUM_THREADS):
        p = Process(target=rethinkdb_insert, args=(queue, lists[i]))
        procs.append(p)
        p.start()

    # Wait for all the checkers to complete
    i = 0
    while(i != NUM_THREADS):
        res = queue.get()
        if res == -1:
            print "Insertion failed, most likely the db isn't running on port %s" % PORT
            map(Process.terminate, procs)
            sys.exit(-1)
        i += 1


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
