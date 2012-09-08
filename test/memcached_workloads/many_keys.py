#!/usr/bin/python
import random, sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import memcached_workload_common
from vcoptparse import *

op = memcached_workload_common.option_parser_for_memcache()
op["num_keys"] = IntFlag("--num-keys", 5000)
op["sequential"] = BoolFlag("--sequential")
op["phase"] = ChoiceFlag("--phase", ["w", "r", "wr"], "wr")
opts = op.parse(sys.argv)

with memcached_workload_common.make_memcache_connection(opts) as mc:

    if "w" in opts["phase"]:
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
        if "r" not in opts["phase"]:
            print "Dumping chosen keys to disk"
            with open("keys", "w") as keys_file:
                keys_file.write(" ".join(keys))

    if "r" in opts["phase"]:
        if "w" not in opts["phase"]:
            print "Loading chosen keys from disk"
            with open("keys", "r") as keys_file:
                keys = keys_file.read().split(" ")
        # Verify everything
        print "Verifying"
        i = 0
        for key in keys:
            if i % 16 == 0:
                values = mc.get_multi(keys[i:i+16])
            value = values[key]
            if value != key:
                raise ValueError("Key %r is set to %r, expected %r" % (key, value, key))
            i += 1

    print "Success"
