#!/usr/bin/python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import memcached_workload_common

op = memcached_workload_common.option_parser_for_memcache()
opts = op.parse(sys.argv)

with memcached_workload_common.make_memcache_connection(opts) as mc:
    
    print "Testing increment"
    if mc.set(str(1),str(1)) == 0:
        raise ValueError, "Set failed"
    mc.incr(str(1),10)
    if mc.get(str(1)) != str(11):
        raise ValueError("simple increment fails, should have been 11, was %s" % mc.get(str(1)))

    # TODO: Problem with Python not being able to handle large unsigned values in the call of incr?
    #if mc.set(str(1),str(1)) == 0:
    #    raise ValueError, "Set failed"
    #mc.incr(str(1),9223372036854775808)
    #if mc.get(str(1)) != str(9223372036854775808):
    #    raise ValueError("large number increment fails, should have been 9223372036854775808, was %s" % mc.get(str(1)))

    #if mc.set(str(1),str(9223372036854775807)) == 0:
    #    raise ValueError, "Set failed"
    #mc.incr(str(1),2)
    #mc.incr(str(1),9223372036854775807)
    #if mc.get(str(1)) != str(0):
    #    raise ValueError("overflow increment fails, should have been 0, was %s" % mc.get(str(1)))

    # TODO: Figure out a way to test negative increments and incrementing by a very large value.
    # memcache doesn't allow either.

    print "Testing decrement"
    if mc.set(str(1),str(50)) == 0:
        raise ValueError, "Set failed"
    mc.decr(str(1),10)
    if mc.get(str(1)) != str(40):
        raise ValueError("simple decrement fails, should have been 40, was %s" % mc.get(str(1)))

    # TODO: Problem with Python not being able to handle large unsigned values in the call of decr?
    #if mc.set(str(1),str(9223372036854775809)) == 0:
    #    raise ValueError, "Set failed"
    #mc.decr(str(1),9223372036854775808)
    #if mc.get(str(1)) != str(1):
    #    raise ValueError("large number decrement fails, should have been 1, was %s" % mc.get(str(1)))

    if mc.set(str(1),str(51)) == 0:
        raise ValueError, "Set failed"
    mc.decr(str(1),52)
    if mc.get(str(1)) != str(0):
        raise ValueError("underflow decrement fails, should have been 0, was %s" % mc.get(str(1)))

    # TODO: Figure out a way to test negative decrements and decrementing by a very large value.
    # memcache doesn't allow either.
