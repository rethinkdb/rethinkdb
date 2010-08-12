#!/usr/bin/python
""" 
    We want to make sure that the data doesn't get corrupted if the server is abruptly killed.
    
    We want to test for:
      - insert keys into the database, and then randomly kill. All the keys we tried
        to insert should have been serialized.
      - check which keys have been stored in the database. There shouldn't be any keys
        that we did not explicitly store in the database (negative test).
      - validate the btree. This should be an on-demand test, probably in c. Make sure
        that we are not violating any properties of btrees.
"""
import pylibmc as memcache
from random import shuffle, randint
from time import sleep
from test_common import *
import os

NUM_INTS = 300
NUM_THREADS = 1
bin = False
ints = range(0, NUM_INTS)
clone = {}

class RethinkDBCorruptionTester(GenericTester):
        
    def __init__(self, test_function, test_function2, test_function3, mode, valgrind = False, timeout = 60):

        self.test_function = test_function
        self.test_function2 = test_function2
        self.test_function3 = test_function3
        self.mode = mode
        self.valgrind = valgrind
        
        # We will abort ourselves when 'self.own_timeout' is hit. retest2 will abort us when
        # 'self.timeout' is hit. 
        self.own_timeout = timeout
        self.timeout = timeout + 15
    
    def test(self):
        
        d = data_file()
        o = SmartTemporaryFile()
        server = test_server(d, o, self.mode, valgrind = self.valgrind)
        print "Running first test..."
        mutual_list = []
        test_thread = threading.Thread(target = self.run_test_function, args = (self.test_function, server.port, mutual_list))
        test_thread.start()
        time.sleep(.001) # give our server a chance to store some values before we kill it.

        # TODO: This should be a SIGKILL, not a SIGINT, but right now if the 
        # server is killed, no data is actually written to the files. When we
        # get the new serializer, we should change this to a SIGKILL and also
        # talk about writing a corruption test more geared towards that specific
        # serializer.
        if not test_thread.is_alive():
            raise RuntimeError, "first test finished before we could kill the server."
        else:
            server.server.send_signal(signal.SIGINT)
            test_thread.join(self.own_timeout)
        
        if len(mutual_list) == 0:
            raise RuntimeError, "server was killed before any values were inserted."
        else:
            last_written_key = mutual_list[-1]

        print "Server killed."
        
        print "Starting another server..."        
        server = test_server(d, o, self.mode, valgrind = self.valgrind)
        print "Running second test..."
        mutual_list2 = []
        test_thread = threading.Thread(target = self.run_test_function, args = (self.test_function2, server.port, mutual_list2))
        test_thread.start()
        test_thread.join(self.own_timeout)
        if len(mutual_list2) == 0:
            last_read_key = -1
        else:
            last_read_key = mutual_list2[-1]
        print "last_read_key",last_read_key
        print "last_written_key",last_written_key

        if test_thread.is_alive():
            self.test_failure = "The integration test didn't finish in time, probably because the " \
                "server wasn't responding to its queries fast enough."
        elif last_read_key < last_written_key - 1:
            self.test_failure = "The last written key was %d, but the last read key was %d" % (last_written_key, last_read_key)
        else: self.test_failure = None
        
        previous_failure = self.test_failure

        print "Running third test..."
        test_thread = threading.Thread(target = self.run_test_function, args = (self.test_function3, server.port, mutual_list))
        test_thread.start()
        test_thread.join(self.own_timeout)
        
        if test_thread.is_alive():
            self.test_failure = "The integration test didn't finish in time, probably because the " \
                "server wasn't responding to its queries fast enough."
        
        if self.test_failure is not None:
            self.test_failure += previous_failure or ""
        else: self.test_failure = previous_failure
        return server.shutdown(self.test_failure)
        
        
def rethinkdb_insert(mc, ints, clone, mutual_list):
    for i in ints:
        print "Inserting %d" % i
        mutual_list.append(i)
        if (0 == mc.set(str(i), str(i))):
            raise ValueError("Insert of %d failed" % i)
        clone[str(i)] = str(i)

        
def rethinkdb_verify(mc, ints, clone, mutual_list):
    """
        Checks to see that all values we inserted into the database
        were stored correctly.
    """
    for i in ints:
        print "Verifying existence of %d" % i
        key = str(i)
        stored_value = mc.get(key)
        actual_value = key
        print "(%s=>%s, should be %s)" % (key, stored_value, actual_value)
        if actual_value != stored_value:
            raise ValueError("Error, incorrect value in the database! (%s=>%s, should be %s). %d keys correctly stored." % \
                (key, stored_value, actual_value, i-1))
        mutual_list.append(i)

def rethinkdb_verify_negative(mc, ints, clone, mutual_list):
    """
        Checks to see that we don't have any extraneous keys in
        our database.
    """
    final_stored_key = mutual_list[-1]
    print "final_stored_key", final_stored_key
#    mutual_list = []
    for i in range(final_stored_key + 1, NUM_INTS):
        print "Verifying non-existence of %d" % i
        key = str(i)
        stored_value = mc.get(key)
        if stored_value != None:
            raise ValueError("Error, key %d exists in the database! It should not exist." % i)
#        mutual_list.append(i)

def test_insert(port, mutual_list):
    mc = memcache.Client(["localhost:%d" % port], binary = bin)
    print "Inserting numbers"
    rethinkdb_insert(mc, ints, clone, mutual_list)
    print "Done"

def test_verify(port, mutual_list):
    mc = memcache.Client(["localhost:%d" % port], binary = bin)
    print "Verifying numbers"
    rethinkdb_verify(mc, ints, clone, mutual_list)    
    print "Done"

def test_verify_negative(port, mutual_list):
    mc = memcache.Client(["localhost:%d" % port], binary = bin)
    print "Verifying non-existence of keys"
    rethinkdb_verify_negative(mc, ints, clone, mutual_list)    
    print "Done"

retest_release = RethinkDBCorruptionTester(test_insert, test_verify, test_verify_negative, "release", True, timeout = 10)
retest_valgrind = RethinkDBCorruptionTester(test_insert, test_verify, test_verify_negative, "debug", True, timeout = 10)

if __name__ == '__main__':
    print "The corruption test involves shutting down and restarting a server, so it can't be run this way."