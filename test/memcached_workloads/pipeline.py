#!/usr/bin/python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import sys, os, random
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import memcached_workload_common, time
from vcoptparse import *

# "I am a string" -> ["I a", "m a s", "trin", "g"]
def rand_split(string, nsub_strings):
    cutoffs = random.sample(range(len(string)), nsub_strings);
    cutoffs.sort();
    cutoffs.insert(0, 0)
    cutoffs.append(len(string))
    strings = []
    for (start, end) in zip(cutoffs[0:len(cutoffs) - 1], cutoffs[1:len(cutoffs)]):
        strings.append(string[start:end])

    return strings

op = memcached_workload_common.option_parser_for_socket()
op["chunk_size"] = IntFlag("--chunk-size", 10)
op["num_ints"] = IntFlag("--num-ints", 1000)
op["num_chunks"] = IntFlag("--num-chunks", 50)
opts = op.parse(sys.argv)

with memcached_workload_common.make_socket_connection(opts) as s:

    ints = range(opts["num_ints"])

    print "Set time"

    command_string = ''
    for int in ints:
        command_string += ("set " + str(int) + " 0 0 " + str(len(str(int))) + " noreply\r\n" + str(int) + "\r\n")

    strings = rand_split(command_string, opts["num_chunks"])
    for string in strings:
        s.send(string)

    start = time.time()
    print "Get time"

    s.send("set foo 0 0 3\r\nbar\r\n")
    s.recv(len("STORED\r\n"))

    #pipeline some gets
    command_string = ''
    expected_response = ''
    lengths = []
    for int in ints:
        cmd = ("get " + str(int) + "\r\n")
        rsp = ("VALUE "+ str(int) + " 0 " + str(len(str(int))) + "\r\n" + str(int) + "\r\nEND\r\n")
        command_string += cmd
        expected_response += rsp
        lengths.append((len(cmd), len(rsp)))

    sent_cmds = 0
    sent_bytes = 0
    response = ''

    foo = 0
    #print len(command_string), len(expected_response)
    while len(response) < len(expected_response):# and response == expected_response[0:len(response)]:
        cur_cmds = sent_cmds
        cur_bytes = sent_bytes
        cmd = ""
        while sent_bytes < len(command_string) and cur_cmds < sent_cmds + opts["chunk_size"]:
            chunk = command_string[cur_bytes : cur_bytes + lengths[cur_cmds][0]]
            cmd += chunk
            cur_bytes += len(chunk)
            cur_cmds += 1

        sent_bytes += s.send(cmd)

        out = ""
        expected = sum([lengths[i][1] for i in range(sent_cmds, cur_cmds)])
        while len(out) < expected:
            out += s.recv(expected - len(out))

        sent_cmds = cur_cmds
        response += out

    print "Finished gets in %f seconds" % (time.time() - start)

    if response != expected_response:
        raise ValueError("Incorrect response: %r Expected: %r" % (response, expected_response))

    '''
    print "Delete time"
    command_string = ''
    expected_response = ''
    for int in ints:
        command_string += ("delete " + str(int) + "\r\n")
        expected_response += "DELETED\r\n"
    
    strings = rand_split(command_string, nChunks)
    for string in strings:
        s.send(string)
    
    response = ''
    while (len(response) < len(expected_response) and response == expected_response[0:len(response)]):
        response += s.recv(len(expected_response) - len(response))
    
    if response != expected_response:
        raise ValueError("Incorrect response: %r Expected: %r" % (response, expected_response))
    '''
    s.send("quit\r\n")
