#!/usr/bin/python
from test_common import *

def test_function(opts, mc):
    
    print "Testing increment"
    if mc.set(str(1),str(1)) == 0:
        raise ValueError, "Set failed"
    mc.incr(str(1),10)
    if mc.get(str(1)) != str(11):
        raise ValueError, "simple increment fails, should have been 11, was", mc.get(str(1))

    # TODO: Figure out a way to test negative increments and incrementing by a very large value.
    # memcache doesn't allow either.

    print "Testing decrement"
    if mc.set(str(1),str(50)) == 0:
        raise ValueError, "Set failed"
    mc.decr(str(1),10)
    if mc.get(str(1)) != str(40):
        raise ValueError, "simple decrement fails, should have been 40, was", mc.get(str(1))

    # TODO: Figure out a way to test negative decrements and decrementing by a very large value.
    # memcache doesn't allow either.

if __name__ == "__main__":
    simple_test_main(test_function, make_option_parser().parse(sys.argv), timeout = 5)
