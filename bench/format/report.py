# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/python
from format import *
import sys
def usage(prog_name):
    print "Usage: %s dbench_output_dir email_to_report_to" % prog_name
if len(sys.argv) != 3:
    usage(sys.argv[0])
    sys.exit(2)
db = dbench(sys.argv[1] + '/', sys.argv[2])
db.report()
