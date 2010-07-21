#!/usr/bin/python

import os
import sys
import subprocess
from multiprocessing import Pool, Queue, Process
import memcache
from random import shuffle, randint
import random
import string

HOST="localhost"
PORT=os.getenv("RUN_PORT", "11211")

# TODO: when we add more integration tests, the act of starting a
# RethinkDB process should be handled by a common external script.

def rethinkdb_set(args, db, clone):
    assert(len(args) == 2)
    print "Setting %s => %s" % args
    result = db.set(*args)
    clone[args[0]] = args[1]
    if (result == 0):
        print "Set failed"
        sys.exit(-1)

def rethinkdb_delete(args, db, clone):
    assert(len(args) == 1)
    print "Deleting %s" % args
    result = db.delete(*args)
    clone[args[0]] = None
    if (result == 0):
        print "Delete failed"
        sys.exit(-1)
   
def rethinkdb_verify(args, db, clone):
    print "Verifying"
    error = False
    for key, clone_value in clone.items():
        stored_value = db.get(key)
        if clone_value != stored_value:
            print "Error, incorrent value in the database! (%s=>%s, should be %s)" % (key, stored_value, clone_value)
            error = True
    if error:
        sys.exit(-1)

def main(argv):
    log_file = sys.argv[1] if len(sys.argv) > 1 else "log.txt"

    clone = {}
    log = open(log_file, 'r')
    action = { "set": rethinkdb_set, "delete": rethinkdb_delete , "verify": rethinkdb_verify}
    mc = memcache.Client([HOST + ":" + PORT], debug=0)
    try:
        for line in log:
            tokens = line.strip().split(None, 2)
            action[tokens[0]](tuple(tokens[1:]), mc, clone)
    except KeyError:
        print "Invalid action specified: %s" % line.strip()
        sys.exit(-1)
            
    rethinkdb_verify(None, mc, clone)
    mc.disconnect_all()
    
    log.close()
    
    print "Replay complete"

if __name__ == '__main__':
    sys.exit(main(sys.argv))
