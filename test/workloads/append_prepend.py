#!/usr/bin/python
import sys, workload_common

op = workload_common.option_parser_for_memcache()
opts = op.parse(sys.argv)

if opts["phase-count"]:
	sys.exit(0)

with workload_common.make_memcache_connection(opts) as mc:

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
