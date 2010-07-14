#!/usr/bin/python
HOST = 'localhost'
PORT = 11213
nInts = 1000
nChunks = 200

from time import sleep
import socket
import random

#split a list into nChunks of lists of random size
#"I am a string" -> ["I a", "m a s", "strin", "g"]
def rand_split(string, nsub_strings):
    cutoffs = random.sample(range(len(string)), nsub_strings);
    cutoffs.sort();
    cutoffs.insert(0, 0)
    cutoffs.append(len(string))
    strings = []
    for (start, end) in zip(cutoffs[0:len(cutoffs) - 1], cutoffs[1:len(cutoffs)]):
        strings.append(string[start:end])

    return strings

ints = range(nInts)

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST, PORT))
command_string = ''
for int in ints:
    command_string += ("set " + str(int) + " 0 0 " + str(len(str(int))) + " noreply\r\n" + str(int) + "\r\n")

strings = rand_split(command_string, nChunks)
for string in strings:
    s.send(string)

sleep(5)

#pipeline some gets
command_string = ''
expected_response = ''
for int in ints:
    command_string += ("get " + str(int) + "\r\n")
    expected_response += ("VALUE "+ str(int) + " 0 " + str(len(str(int))) + "\r\n" + str(int) + "\r\nEND\r\n")

strings = rand_split(command_string, nChunks)
for string in strings:
    s.send(string)

sleep(1) #apparently we need to sleep before we poll (probably because I don't understand how to make these socket calls)
response = s.recv(len(expected_response))
if (response != expected_response):
    print "Incorrect response:\n", response, "\nExpected:\n", expected_response

command_string = ''
expected_response = ''
for int in ints:
    command_string += ("delete " + str(int) + "\r\n")
    expected_response += "DELETED\r\n"

strings = rand_split(command_string, nChunks)
for string in strings:
    s.send(string)

sleep(1) #apparently we need to sleep before we poll (probably because I don't understand how to make these socket calls)
response = s.recv(len(expected_response))
if (response != expected_response):
    print "Incorrect response:\n", response, "\nExpected:\n", expected_response

s.send("quit\r\n")
s.close()
#import memcache
#mc = memcache.Client([(HOST, PORT)])
#for int in ints:
#    print "Checking %d" % int
#    if mc.get(str(int)) != str(int):
#        print "Incorrect value in the db: %s => %s" % (str(int), mc.get(str(int)))
#        import sys
#        sys.exit(-1)
