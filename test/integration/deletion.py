#!/usr/bin/python

import sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

def mid(a):
    half = len(a) / 2
    for j in xrange(half):
        yield a[half - j - 1]
        yield a[half + j]

reorder_funs = {
    'fwd': lambda x: x,
    'rev': reversed,
    'mid': mid,
    'midrev': lambda x: mid(list(reversed(x)))
}

def test(opts, mc, test_dir):
    keys = [("%0" + str(opts['key_len']) + "d") % (key,) for key in xrange(opts['max_key']+1)]
    val = 'Q' * opts['val_len']

    print "Inserting"
    for k in keys:
        mc.set(k, val)

    print "Deleting"
    for k in reorder_funs[opts['pattern']](keys):
        mc.delete(k)

    print "Done"

# This test is written somewhat oddly because I was trying to reproduce my
# earlier manual tests exactly.
# The general goal is to trigger all the various edge cases of the leveling
# code.

if __name__ == "__main__":
    op = make_option_parser()
    op['max_key'] = IntFlag("--max-key", 1000)
    op['key_len'] = IntFlag("--key-len", 4)
    op['val_len'] = IntFlag("--val-len", 45)
    op['pattern'] = ChoiceFlag("--pattern", reorder_funs.keys(), "fwd")

    opts = op.parse(sys.argv)
    simple_test_main(test, opts, timeout = 2400)
    # After we run this on EC2 once, the actual timeout can be reduced to however long it took to run.
