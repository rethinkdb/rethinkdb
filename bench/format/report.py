#!/usr/bin/python
from format import *
import sys
def usage(prog_name):
    print "Usage: %s dbench_output_dir" % prog_name
if len(sys.argv) != 2:
    usage(sys.argv[0])
    sys.exit(2)
db = dbench(sys.argv[1])
db.report()
