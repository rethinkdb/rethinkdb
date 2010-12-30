#!/usr/bin/python
import os, time, random, threading, sys
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

if __name__ == "__main__":

    op = make_option_parser()
    op["mclib"].default = "memcache"   # memcache plays nicer with this test than pylibmc does
    op["num_keys"] = IntFlag("--num-keys", 1000)
    opts = op.parse(sys.argv)

    make_test_dir()

    # Start server

    server = Server(opts)
    server.start()

    # Insert many keys, then remove them all

    mc = connect_to_port(opts, server.port)

    print "Inserting keys..."

    for i in xrange(opts["num_keys"]):
        ok = mc.set(str(i), str(i))
        if ok == 0: raise ValueError("Could not insert %r" % i)

    print "Waiting for a flush..."

    time.sleep(5)

    print "Removing keys..."

    for i in xrange(opts["num_keys"]):
        ok = mc.delete(str(i))
        if ok == 0: raise ValueError("Could not delete %r" % i)

    mc.disconnect_all()

    # Restart server

    server.shutdown()
    server.start()

    # Make sure no keys are present

    mc = connect_to_port(opts, server.port)

    print "Checking keys..."

    for i in xrange(opts["num_keys"]):
        assert not mc.get(str(i))

    # Clean up
    
    server.shutdown()

