#!/usr/bin/python

import pylibmc as memcache
from random import shuffle, randint
from time import sleep
import os
import socket

NUM_INTS = 5
NUM_THREADS = 1
bin = True

def rethinkdb_multi_insert(mc, ints, clone):
    insert_dict = {}
    for i in ints:
        insert_dict[str(i)] = str(i)
        clone[str(i)] = str(i)
    if (0 == mc.set_multi(insert_dict)):
        raise ValueError("Multi insert failed")


def rethinkdb_multi_delete(mc, ints, clone):
    deletes = []
    for i in ints:
        deletes.append(str(i))

    deletes_lists = split_list(deletes, randint(NUM_INTS // 40, NUM_INTS // 5))

    for list in deletes_lists:
        mc.delete_multi(list)
        for i in list:
            del clone[str(i)]

    print "Verifying"
    rethinkdb_multi_verify(mc, ints, clone)

def get_pipelined(socket):
    if (NUM_INTS < 5):
        raise ValueError("Sorry the test needs to be run with NUM_INTS > 5")
    #we did mention this was jank didn't we?
    #sends 5 getkqs (0,1,2,3,4) and a noop
    socket.send("\x80\x0d\x00\x01\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x34\x80\x0d\x00\x01\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x31\x80\x0d\x00\x01\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x33\x80\x0d\x00\x01\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x32\x80\x0d\x00\x01\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x30\x80\x0a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00")

    expected_response = "\x81\x0d\x00\x01\x04\x00\x00\x00\x00\x00\x00\x06\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x34\x34\x81\x0d\x00\x01\x04\x00\x00\x00\x00\x00\x00\x06\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x31\x31\x81\x0d\x00\x01\x04\x00\x00\x00\x00\x00\x00\x06\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x33\x33\x81\x0d\x00\x01\x04\x00\x00\x00\x00\x00\x00\x06\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x32\x32\x81\x0d\x00\x01\x04\x00\x00\x00\x00\x00\x00\x06\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x30\x30\x81\x0a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    actual_response = socket.recv(len(expected_response))

    if actual_response != expected_response:
        raise ValueError("Pipelined get failed, jank test so I can't tell you any more about what happened")

def split_list(alist, parts):
    length = len(alist)
    return [alist[i * length // parts: (i + 1) * length // parts]
            for i in range(parts)]

def test_against_server_at(port):
    mc = memcache.Client(["localhost:%d" % port], binary = bin)
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("localhost", port))

    clone = {}
    print "====Testing multi operations===="

    ints = range(0, NUM_INTS)
    shuffle(ints)

    print "Inserting numbers (multi)"
    rethinkdb_multi_insert(mc, ints, clone)
    mc.disconnect_all()

    print "Verifying (multi)"
    get_pipelined(s)

#print "Deleting numbers (multi)"
#rethinkdb_multi_delete(mc, ints, clone)
    
    print "Done"

from test_common import RethinkDBTester
retest_release = RethinkDBTester(test_against_server_at, "release")
retest_valgrind = RethinkDBTester(test_against_server_at, "debug", valgrind=True)

if __name__ == '__main__':
    test_against_server_at(int(os.environ.get("RUN_PORT", "11211")))
