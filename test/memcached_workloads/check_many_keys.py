#!/usr/bin/python
import random, sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import memcached_workload_common
from vcoptparse import *

op = memcached_workload_common.option_parser_for_memcache()
op["num_keys"] = IntFlag("--num-keys", 5000)
op["sequential"] = BoolFlag("--sequential")
opts = op.parse(sys.argv)

with memcached_workload_common.make_memcache_connection(opts) as mc:
    keys = [str(x) for x in xrange(opts["num_keys"])]
    # Verify everything
    print "Verifying"
    i = 0
    values = mc.get_multi(keys[i:i+16])
    for key in keys:
        if i % 16 == 0:
            values = mc.get_multi(keys[i:i+16])
            
        value = values[key]
        if value != key:
            raise ValueError("Key %r is set to %r, expected %r" % (key, value, key))
        i += 1
    
    print "Success"
