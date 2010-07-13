#!/usr/bin/python

import sys
import subprocess
from multiprocessing import Pool
from multiprocessing import Process
from multiprocessing import Queue
import pylibmc as memcache
from random import randint
from time import time

NUM_KEYS=1000           # Total number of key to insert
NUM_VALUES=20           # Number of values a given key can assume

TEST_DURATION=15        # Duration of the test in seconds
NUM_READ_THREADS=64     # Number of concurrent reader threads
NUM_UPDATE_THREADS=16   # Number of concurrent threads updating existing values
NUM_INSERT_THREADS=8    # Number of concurrent threads inserting values that aren't yet in the db

# Server location
HOST="localhost"
PORT="11213"
bin = False

def insert_initial(matrix):
    mc = memcache.Client([HOST + ":" + PORT], binary = bin)
    j = 0
    for row in matrix:
        mc.set(str(j), str(row[0]))
        j += 1
    mc.disconnect_all()

def cycle_values(matrix):
    mc = memcache.Client([HOST + ":" + PORT], binary = bin)
    time_start = time()
    while True:
        # Set a random key to one of its permitted values
        j = randint(0, NUM_KEYS - 1)
        mc.set(str(j), str(matrix[j][randint(0, NUM_VALUES - 1)]))
        # Disconnect if our time is out
        if(time() - time_start > TEST_DURATION):
            mc.disconnect_all()
            return

def check_values(queue, matrix):
    mc = memcache.Client([HOST + ":" + PORT], binary = bin)
    time_start = time()
    while True:
        # Get a random key and make sure its value is in the permitted range
        j = randint(0, NUM_KEYS - 1)
        value = mc.get(str(j))
        if (not value):
            queue.put(-1)
            print "A key (%d) is missing a value" % j
            return
        if (not (int(value) in matrix[j])):
            queue.put(-1)
            print "A key (%d) has incorrect value" % j
            return
        # Disconnect if our time is out
        if(time() - time_start > TEST_DURATION):
            mc.disconnect_all()
            queue.put(0)
            return

def insert_values(_):
    mc = memcache.Client([HOST + ":" + PORT], binary = bin)
    time_start = time()
    j = NUM_KEYS
    while True:
        # Set a random key to one of its permitted values
        mc.set(str(j), str(j))
        j += 1
        # Disconnect if our time is out
        if(time() - time_start > TEST_DURATION):
            mc.disconnect_all()
            return

def main(argv):
    # Generate a matrix of values a given key can assume
    print "Generating values"
    matrix = [[randint(0, 1000000) for _ in xrange(0, NUM_VALUES)] for x in xrange(0, NUM_KEYS)]

    # Insert the first value for each key
    print "Insert initial values"
    insert_initial(matrix)

    # Start cycling and checking
    if NUM_UPDATE_THREADS:
        print "Start cycling values"
        p_cyclers = Pool(NUM_UPDATE_THREADS)
        p_cyclers.map_async(cycle_values, [matrix for _ in xrange(0, NUM_UPDATE_THREADS)])
        p_cyclers.close()

    if NUM_INSERT_THREADS:
        print "Start inserting values"
        p_inserters = Pool(NUM_INSERT_THREADS)
        p_inserters.map_async(insert_values, [0 for _ in xrange(0, NUM_INSERT_THREADS)])
        p_inserters.close()
        
    print "Start checking values"
    queue = Queue()
    procs = []
    for i in xrange(0, NUM_READ_THREADS):
        p = Process(target=check_values, args=(queue, matrix))
        procs.append(p)
        p.start()

    # Wait for all the checkers to complete
    i = 0
    while(i != NUM_READ_THREADS):
        res = queue.get()
        if res == -1:
            map(Process.terminate, procs)
            sys.exit(-1)
        i += 1

    # Wait for all the updates to complete
    if NUM_UPDATE_THREADS:
        p_cyclers.join()
    if NUM_INSERT_THREADS:
        p_inserters.join()

if __name__ == '__main__':
    sys.exit(main(sys.argv))
