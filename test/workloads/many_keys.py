#!/usr/bin/python
import random, sys, workload_common
from vcoptparse import *

op = workload_common.option_parser_for_memcache()
op["num_keys"] = IntFlag("--num-keys", 5000)
op["sequential"] = BoolFlag("--sequential")
opts = op.parse(sys.argv)

if opts["phase-count"]:
	sys.exit(0)

with workload_common.make_memcache_connection(opts) as mc:
    
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
            raise ValueError("Could not set %r" % key)
        i += 1

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
