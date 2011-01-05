#!/usr/bin/python
""" 
    We want to make sure that the data remains valid as the server is killed and
    starts back up again. Ideally we would also like to validate the btree, but that should probably
    be done in C.
"""
import sys, os, time, random, threading
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

if __name__ == "__main__":
    
    op = make_option_parser()
    op["mclib"].default = "memcache"   # memcache plays nicer with this test than pylibmc does
    opts = op.parse(sys.argv)
    
    test_dir = TestDir()
    mutual_list = []
    
    # Start first server
    
    opts_without_valgrind = dict(opts)
    opts_without_valgrind["valgrind"] = False
    server1 = Server(opts_without_valgrind, name="first server", extra_flags = ["--wait-for-flush", "y", "--flush-timer", "0"], test_dir = test_dir)
    server1.start()
    server_killed = False
    
    # Run procedure to insert some values into database
    
    print "Running first test..."
    def first_test():
        mc = connect_to_port(opts, server1.port)
        while not server_killed:
            value = str(random.randint(0, 10000))
            ok = mc.set(value, value)
            if ok == 0:
                if server_killed: break
                else: raise ValueError("Could not insert %r" % value)
            mutual_list.append(value)
        mc.disconnect_all()   # At this point the server is dead anyway, but what the hell
    th = threading.Thread(target = first_test)
    th.start()
    th.join(2)
    
    # Make sure nothing went wrong in insertion of values
    
    if not th.is_alive():
        print "Inserter thread died prematurely; we will not go on to the next phase of the test."
        server1.shutdown()
        sys.exit(1)
    
    # Kill first server
    
    server_killed = True
    server1.kill()
    
    # Store snapshot of possibly-corrupted data files
    
    print
    snapshot_dir = test_dir.p("db_data_snapshot")
    print "Storing snapshot of data files in %r." % snapshot_dir
    shutil.copytree(test_dir.p("db_data"), snapshot_dir)
    print
    
    # Start second server
    
    server2 = Server(opts, name="second server", data_files = server1.data_files, test_dir = test_dir)
    server2.start()
    
    # Check to make sure that the values are all present
    
    def second_test(test_dir):
        mc = connect_to_port(opts, server2.port)
        successes = fails = 0
        for value in mutual_list:
            value2 = mc.get(value)
            if value == value2: successes += 1
            elif value2 is None: fails += 1
            else:
                # If the value was not merely missing, but actually turned into a different value,
                # then raise an error saying that
                raise ValueError("Expected %s, got %s" % (repr(value), repr(value2)))
        mc.disconnect_all()
        print "Out of %d values written, %d survived the server shutdown." % \
            (len(mutual_list), successes)
        if fails > 10:   # Arbitrary threshold above which to declare the test a failure
            raise ValueError("Unacceptably many values were missing.")
    run_with_timeout(second_test, timeout = 30, name = "second test", test_dir = test_dir)
    
    # Clean up
    
    server2.shutdown()

