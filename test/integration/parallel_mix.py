#!/usr/bin/python

import sys
import subprocess
from multiprocessing import Pool
import memcache
from random import randint
from time import time

NUM_KEYS=1000          # Total number of key to insert
NUM_VALUES=20          # Number of values a given key can assume

TEST_DURATION=5        # Duration of the test in seconds
NUM_READ_THREADS=64    # Number of concurrent reader threads
NUM_WRITE_THREADS=16   # Number of concurrent writer threads

# Server location
HOST="localhost"
PORT="11211"

def insert_initial(matrix):
    mc = memcache.Client([HOST + ":" + PORT], debug=0)
    j = 0
    for row in matrix:
        mc.set(str(j), str(row[0]))
        j += 1
    mc.disconnect_all()

def cycle_values(matrix):
    mc = memcache.Client([HOST + ":" + PORT], debug=0)
    time_start = time()
    while True:
        # Set a random key to one of its permitted values
        j = randint(0, NUM_KEYS - 1)
        mc.set(str(j), str(matrix[j][randint(0, NUM_VALUES - 1)]))
        # Disconnect if our time is out
        if(time() - time_start > TEST_DURATION):
            mc.disconnect_all()
            return

def check_values(matrix):
    mc = memcache.Client([HOST + ":" + PORT], debug=0)
    time_start = time()
    while True:
        # Get a random key and make sure its value is in the permitted range
        j = randint(0, NUM_KEYS - 1)
        value = mc.get(str(j))
        if (not value) or (not (int(value) in matrix[j])):
            print "A key (%d) has an incorrect or missing value" % j
            sys.exit(-1)
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

    print "Start cycling values"
    cycle_values(matrix)
    print "Start checking values"
    check_values(matrix)

    
if __name__ == '__main__':
    sys.exit(main(sys.argv))
