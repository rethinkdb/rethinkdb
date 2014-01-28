#!/usr/bin/python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import time, sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import memcached_workload_common

def expect(b, msg):
    if (not b):
        raise ValueError, msg

def typical_test(mc, k, t, first_sleep, second_sleep):
    expect(mc.set(k, "aaa", time=t) != 0, "Set failed")
    print "   Make sure we can get the element back after a short sleep"
    time.sleep(first_sleep)
    expect(mc.get(k) == "aaa",
           "Failure: value can't be found but it's supposed to be")

    print "   Make sure the element eventually expires"
    time.sleep(second_sleep)
    expect(mc.get(k) == None,
           "Failure: value should have expired but it didn't")

# Not really an exptime test.
def zero_test(mc):
    print "zero_test..."
    expect(mc.set("z", "aaa", time=0) != 0, "Set failed")
    print "   Make sure we can get the element back."
    expect(mc.get("z") == "aaa", "Value can't be found, hey we didn't even set the exptime!")
    print "   Done zero_test."

# Tests an absolute timestamp 5 seconds from now.
def absolute_test(mc):
    print "absolute_test..."
    t = int(time.time()) + 5
    typical_test(mc, "b", t, 1, 4)
    print "   Done absolute_test."

# Tests a relative timestamp 5 seconds from now.
def basic_test(mc):
    print "basic_test..."
    typical_test(mc, "a", 5, 1, 4)
    print "   Done basic_test."

# Tests an expiration time that's already in the past.
def past_test(mc):
    print "past_test..."
    expect(mc.set("p", "aaa", time=int(time.time()) - 5) == 1, "Set failed")
    print "   Make sure we can't get the element back."
    expect(mc.get("p") == None, "Wait, we got a value?!")
    print "   Done past_test."

op = memcached_workload_common.option_parser_for_memcache()
opts = op.parse(sys.argv)

with memcached_workload_common.make_memcache_connection(opts) as mc:
    zero_test(mc)
    basic_test(mc)
    absolute_test(mc)
    past_test(mc)

    print "Done"
