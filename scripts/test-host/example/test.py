# Copyright 2010-2012 RethinkDB, all rights reserved.
import time, random, sys

with open("log.txt", "w") as log:
    for i in xrange(60):
        value = random.random()
        print >>log, value
        print value
        log.flush()
        sys.stdout.flush()
        time.sleep(1)
