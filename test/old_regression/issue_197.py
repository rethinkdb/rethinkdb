#!/usr/bin/python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import os, sys, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, os.path.pardir, 'bench', 'stress-client')))
try:
    import stress
except:
    print "This test needs the Python interface to the stress client."
    raise

def test_function(opts, port, test_dir):

    model = stress.FuzzyModel(1)   # Every single operation will hit the same key
    key_generator = stress.SeedKeyGenerator()

    print "Setting up connections..."

    read_client = stress.Client()
    read_connection = stress.Connection("localhost:%d" % port)
    read_op = stress.ReadOp(key_generator, model.random_chooser(), read_connection, batch_factor=1)
    read_client.add_op(1, read_op)
    write_client = stress.Client()
    write_connection = stress.Connection("localhost:%d" % port)
    write_op = stress.UpdateOp(key_generator, model.random_chooser(), None, write_connection, size=1000)
    write_client.add_op(1, write_op)

    duration = 10
    print "Running for %d seconds..." % duration

    read_client.start()
    write_client.start()
    time.sleep(duration)
    read_client.stop()
    write_client.stop()

    print "Did %d inserts and %d reads." % (write_op.poll()["queries"], read_op.poll()["queries"])

if __name__ == "__main__":
    op = make_option_parser()
    auto_server_test_main(test_function, op.parse(sys.argv))
    