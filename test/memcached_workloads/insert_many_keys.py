#!/usr/bin/python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import random, sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import memcached_workload_common
from vcoptparse import *

op = memcached_workload_common.option_parser_for_memcache()
op["num_keys"] = IntFlag("--num-keys", 5000)
op["sequential"] = BoolFlag("--sequential")
opts = op.parse(sys.argv)

with memcached_workload_common.make_memcache_connection(opts) as mc:
    
    print "Inserting"
    keys = [str(x) for x in xrange(opts["num_keys"])]
    if not opts["sequential"]:
        random.shuffle(keys)
    
    i = 0
    for key in keys:
        # if (i % 500 == 0 or (i < 500 and (i & (i - 1)) == 0)):
        #     print i
        ok = mc.set(key, key)
        if ok == 0:
            print "Failed to set a value"
            raise ValueError("Could not set %r" % key)
        i += 1
