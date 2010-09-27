#!/usr/bin/python
from random import shuffle, randint
from test_common import *

def test(opts, mc):

    print "Shuffling numbers"
    ints = range(0, opts["num_ints"])
    shuffle(ints)

    print "Checking cas on numbers"
    
    with StdoutAsLog("cas_log.txt"):
        for i in ints:
            print "Inserting %d" % i
            if (0 == mc.set(str(i), str(i))):
                raise ValueError("Insert of %d failed" % i)
                
            print "Getting %d" % i
            value, _, cas_id = mc.explicit_gets(str(i))
            if (value != str(i)):
                raise ValueError("get failed, should be %d=>%d, was %s" % (i, i, value))
                
            print "'cas'-ing %d" % i
            if (0 == mc.explicit_cas(str(i), str(i+1), cas_id)):
                raise ValueError("cas of %d failed" % i)
                
            print "Verifying cas %d" % i
            value, _, cas_id = mc.explicit_gets(str(i))
            if (value != str(i+1)):
                raise ValueError("get for cas failed, should be %d=>%d, was %s" % (i, i+1, value))
                
            print "Modifying %d again" % i
            if (0 == mc.set(str(i), str(i+10))):
                raise ValueError("Modify of %d failed" % i)
                
            print "'cas'-ing %d again" % i
            if (0 != mc.explicit_cas(str(i), str(i+20), cas_id)):
                raise ValueError("cas of %d should have failed, item has been modified" % i)

if __name__ == "__main__":
    op = make_option_parser()
    del op["mclib"]   # No longer optional; we only work with memcache.
    op["num_ints"] = IntFlag("--num-ints", 10)
    opts = op.parse(sys.argv)
    opts["mclib"] = "memcache"
    simple_test_main(test, opts, timeout = 5)
