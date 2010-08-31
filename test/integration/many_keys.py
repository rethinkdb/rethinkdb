#!/usr/bin/python
from test_common import *
import random

def test_function(opts, mc):
    
    keys = [str(x) for x in xrange(opts["num_keys"])]
    if not opts["sequential"]:
        random.shuffle(keys)
    
    for key in keys:
        ok = mc.set(key, key)
        if ok == 0:
            raise ValueError("Could not set %r" % key)

if __name__ == "__main__":
    op = make_option_parser()
    op["num_keys"] = IntFlag("--num-keys", 5000)
    op["sequential"] = BoolFlag("--sequential")
    opts = op.parse(sys.argv)
    
    # Adjust the time and memory allocated to the server based on the size of the test. This test is to
    # test the btree, not the serializer, so we try to give the server enough memory to hold the entire
    # tree.
    timeout = opts["num_keys"] * 0.005
    extra_flags = ["-m", str(opts["num_keys"] // 400 + 1)]
    opts["cores"] = opts["slices"] = 1
    
    simple_test_main(test_function, opts, timeout, extra_flags)
