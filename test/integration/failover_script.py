#!/usr/bin/python
import random, time, sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

up_file = "failover_test_elb_up"
down_file = "failover_test_elb_down"

def test(opts, server, repli_server, test_dir):
    time.sleep(10) # TODO: why is this necessary?

    os.system("%s reset" % opts["failover-script"])
    if (os.path.exists(up_file) or os.path.exists(down_file)):
        raise ValueError("The files that the failover script is expected to create already exists and weren't removed when the scripts reset was called I can't work under these conditions.")

    server.shutdown()

    time.sleep(5)

    if (not os.path.exists(down_file)):
        raise ValueError("Took the server down and it didn't create the file: %s... this is unacceptable" % down_file)

    server.start()

    time.sleep(5)

    if (not os.path.exists(up_file)):
        raise ValueError("Took the server up and it didn't create the file: %s... this is unacceptable" % up_file)

    os.system("%s reset" % opts["failover-script"])

if __name__ == "__main__":
    op = make_option_parser().parse(sys.argv)
    if os.path.split(op["failover-script"])[1] != "failover_script_test.py":
        raise ValueError("This should be run with test/assets/failover_script_test.py as is failover script") #is there a none path dependent way to specify this script?
    master_slave_main(test, op, timeout = (100 if not op.has_key("duration") else op["duration"] + 60))
