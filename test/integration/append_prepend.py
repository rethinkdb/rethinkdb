#!/usr/bin/python
from test_common import *

def test_function(opts, mc):

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
    mc.disconnect_all()

if __name__ == "__main__":
    simple_test_main(test_function, make_option_parser().parse(sys.argv), timeout = 2)
