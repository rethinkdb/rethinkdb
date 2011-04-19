#!/usr/bin/python
import os, sys
up_file = "failover_test_elb_up"
down_file = "failover_test_elb_down"

def usage():
    print "Usage"
    print "%s argument" % sys.argv[0]
    print "Values for argument are:"
    print "     down: for when the master goes down"
    print "     up: for when the master comes up"
    print "     reset: remove the files that indicate the script was called"

if __name__ == "__main__":
    if (len(sys.argv) != 2):
        usage()
        exit(-2)
    if (sys.argv[1] == "down"):
        os.system("touch %s" % down_file) #6 points!!
    elif (sys.argv[1] == "up"):
        os.system("touch %s" % up_file)
    elif (sys.argv[1] == "reset"):
        try:
            os.remove(up_file)
        except:
            pass

        try:
            os.remove(down_file)
        except:
            pass
