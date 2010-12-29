#!/usr/bin/python
from test_common import *
import time

def test_function(opts, mc):

    print "Testing set with expiration"
    if mc.set("a", "aaa", time=5) == 0:
        raise ValueError, "Set failed"

    print "Make sure we can get the element back after short sleep"
    time.sleep(1)
    if mc.get("a") != "aaa":
        raise ValueError("Failure: value can't be found but it's supposed to be")
        
    print "Make sure the element eventually expires"
    time.sleep(4)
    if mc.get("a") != None:
        raise ValueError("Failure: value should have expired but didn't")
        
    print "Done"
    mc.disconnect_all()

if __name__ == "__main__":
    simple_test_main(test_function, make_option_parser().parse(sys.argv), timeout = 7)
