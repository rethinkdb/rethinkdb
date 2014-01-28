#!/usr/bin/python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import memcached_workload_common

op = memcached_workload_common.option_parser_for_memcache()
opts = op.parse(sys.argv)

with memcached_workload_common.make_memcache_connection(opts) as mc:

    print "Testing append"
    if mc.set("a", "aaa") == 0:
        raise ValueError, "Set failed"
    mc.append("a", "bbb")
    if mc.get("a") != "aaabbb":
        raise ValueError("Append failed, expected %r, got %r", "aaabbb", mc.get("a"))

    print "Testing prepend"
    if mc.set("a", "aaa") == 0:
        raise ValueError, "Set failed"
    mc.prepend("a", "bbb")
    if mc.get("a") != "bbbaaa":
        raise ValueError("Append failed, expected %r, got %r", "bbbaaa", mc.get("a"))

    print "Done"
