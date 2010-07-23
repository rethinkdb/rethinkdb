#!/usr/bin/python

from multiprocessing import Pool
from multiprocessing import Process
from multiprocessing import Queue
from random import randint
from time import time
import memcache

NUM_KEYS=1000           # Total number of key to insert
NUM_VALUES=20           # Number of values a given key can assume

TEST_DURATION=15        # Duration of the test in seconds
NUM_READ_THREADS=16     # Number of concurrent reader threads
NUM_UPDATE_THREADS=8   # Number of concurrent threads updating existing values
NUM_INSERT_THREADS=4    # Number of concurrent threads inserting values that aren't yet in the db

def insert_initial((port, matrix)):
    mc = memcache.Client(["localhost:%d" % port])
    for j, row in enumerate(matrix):
        print "Initially inserting %s = %s" % (j, row[0])
        ok = mc.set(str(j), str(row[0]))
        if ok == 0: raise ValueError("Set of %s failed" % j)
    mc.disconnect_all()

def cycle_values((port, matrix)):
    mc = memcache.Client(["localhost:%d" % port])
    time_start = time()
    while True:
        # Set a random key to one of its permitted values
        j = randint(0, NUM_KEYS - 1)
        print "Cycling %s" % j
        ok = mc.set(str(j), str(matrix[j][randint(0, NUM_VALUES - 1)]))
        if ok == 0: raise ValueError("Cannot insert %d" % j)
        # Disconnect if our time is out
        if(time() - time_start > TEST_DURATION):
            mc.disconnect_all()
            return

def check_values((port, matrix)):
    print "Checker started"
    mc = memcache.Client(["localhost:%d" % port])
    time_start = time()
    while True:
        # Get a random key and make sure its value is in the permitted range
        j = randint(0, NUM_KEYS - 1)
        print "Checking %s" % j
        value = mc.get(str(j))
        if not value: raise ValueError("A key (%d) is missing a value" % j)
        if int(value) not in matrix[j]: raise ValueError("A key (%d) has incorrect value" % j)
        # Disconnect if our time is out
        if(time() - time_start > TEST_DURATION):
            mc.disconnect_all()
            return

def insert_values(port):
    mc = memcache.Client(["localhost:%d" % port])
    time_start = time()
    j = NUM_KEYS
    while True:
        print "Inserting %s" % j
        ok = mc.set(str(j), str(j))
        if ok == 0: raise ValueError("Set of %s failed" % j)
        j += 1
        # Disconnect if our time is out
        if(time() - time_start > TEST_DURATION):
            mc.disconnect_all()
            return

def test_against_server_at(port):

    # Generate a matrix of values a given key can assume
    matrix = [[randint(0, 1000000) for _ in xrange(0, NUM_VALUES)] for x in xrange(0, NUM_KEYS)]

    # Insert the first value for each key
    print "Inserting initial values"
    insert_initial((port, matrix))

    # Start subprocesses
    print "Starting cyclers"
    p_cyclers = Pool(NUM_UPDATE_THREADS)
    cycler_results = p_cyclers.map_async(cycle_values, [(port, matrix) for _ in xrange(0, NUM_UPDATE_THREADS)])
    p_cyclers.close()

    print "Starting inserters"
    p_inserters = Pool(NUM_INSERT_THREADS)
    inserter_results = p_inserters.map_async(insert_values, [port for _ in xrange(0, NUM_INSERT_THREADS)])
    p_inserters.close()
    
    print "Starting checkers"
    p_checkers = Pool(NUM_READ_THREADS)
    checker_results = p_checkers.map_async(check_values, [(port, matrix) for _ in xrange(0, NUM_READ_THREADS)])
    p_checkers.close()

    # Wait for all the subprocesses to complete. We don't actually care about the result, only
    # whether an exception was raised or not. We use get() instead of wait() because get() re-raises
    # exceptions from subprocesses.
    cycler_results.get()
    inserter_results.get()
    checker_results.get()
    
    print "Done"

from test_common import RethinkDBTester
retest_release = RethinkDBTester(test_against_server_at, "release")
retest_valgrind = RethinkDBTester(test_against_server_at, "debug", valgrind=True)

if __name__ == '__main__':
    test_against_server_at(int(os.environ.get("RUN_PORT", "11211")))