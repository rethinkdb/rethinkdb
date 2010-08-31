#!/usr/bin/python
import os, sys, socket, random
from test_common import *

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

def test_function(opts, port):
    
    ints = range(opts["num_ints"])
    
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("localhost", port))
    
    print "Set time"
    
    command_string = ''
    for int in ints:
        command_string += ("set " + str(int) + " 0 0 " + str(len(str(int))) + " noreply\r\n" + str(int) + "\r\n")
    
    strings = rand_split(command_string, opts["num_chunks"])
    for string in strings:
        s.send(string)
    
    print "Get time"
    
    #pipeline some gets
    command_string = ''
    expected_response = ''
    for int in ints:
        command_string += ("get " + str(int) + "\r\n")
        expected_response += ("VALUE "+ str(int) + " 0 " + str(len(str(int))) + "\r\n" + str(int) + "\r\nEND\r\n")
    
    strings = rand_split(command_string, opts["num_chunks"])
    for string in strings:
        s.send(string)
    
    response = ''
    while (len(response) < len(expected_response) and response == expected_response[0:len(response)]):
        response += s.recv(len(expected_response) - len(response))
    
    if response != expected_response:
        raise ValueError("Incorrect response: %r Expected: %r" % (response, expected_response))
    
    print "Delete time"
    '''
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
    s.close()

if __name__ == "__main__":
    op = make_option_parser()
    op["num_ints"] = IntFlag("--num-ints", 1000)
    op["num_chunks"] = IntFlag("--num-chunks", 50)
    auto_server_test_main(test_function, op.parse(sys.argv), timeout = 5)
