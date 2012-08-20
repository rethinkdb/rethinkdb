#!/usr/bin/python
from random import shuffle
import sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import memcached_workload_common
from vcoptparse import *

op = memcached_workload_common.option_parser_for_memcache()
del op["mclib"]   # No longer optional; we only work with memcache.
op["num_ints"] = IntFlag("--num-ints", 10)
opts = op.parse(sys.argv)
opts["mclib"] = "memcache"

with memcached_workload_common.make_memcache_connection(opts) as mc:

    print "Shuffling numbers"
    ints = range(0, opts["num_ints"])
    shuffle(ints)

    # flag values are:
    #   - 0 for strings
    #   - 2 for ints
    
    print "Testing with flags"
    for i in ints:
        print "Inserting %d" % i
        if i % 2: val = str(i)
        else: val = i
            
        if (0 == mc.set(str(i), val)):
            raise ValueError("Insert of %d failed" % i)
            
        print "Getting %d" % i
        value, flags, _ = mc.explicit_gets(str(i))
        if (value != val):
            raise ValueError("get failed, should be %d=>%d, was %s" % (i, val, value))
        
        print "Checking flag for %d" % i
        if i % 2:
            if flags != 0:
                raise ValueError("flag set failed, should be %d, was %d" % (0, flags))
        else:        
            if flags != 2:
                raise ValueError("flag set failed, should be %d, was %d" % (2, flags))
