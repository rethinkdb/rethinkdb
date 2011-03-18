#!/usr/bin/python
import sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

cache_max_size = 5 * 1024 * 1024
block_size = 4096

def test(opts, mc, test_dir):

    value = "a" * 10 * 1024
    num_keys = cache_max_size / len(value) * 2 + 1

    # Insert twice as many keys as can fit in the cache
    print "Inserting %d keys of %d bytes each..." % (num_keys, len(value))
    for i in xrange(num_keys):
        mc.set(str(i), value)

    # Make sure that only half of those keys still exist
    print "Getting those keys back..."
    existing_keys = 0
    for i in xrange(num_keys):
        v = mc.get(str(i))
        if v is not None:
            existing_keys += 1

    print "Found %d of %d keys." % (existing_keys, num_keys)
    assert existing_keys <= num_keys / 2

if __name__ == "__main__":
    simple_test_main(test, make_option_parser().parse(sys.argv), timeout = 10,
        extra_flags=["--max-cache-size", str(cache_max_size/block_size)])
