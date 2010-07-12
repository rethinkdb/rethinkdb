#!/usr/bin/python

HOST = 'localhost'
PORT = 11213
nInts = 250

import socket
import random

ints = range(nInts)

random.shuffle(ints);
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST, PORT))
command_string = ''
for int in ints:
    command_string += ("set " + str(int) + " 0 0 " + str(len(str(int))) + " noreply\r\n" + str(int) + "\r\n")

print "Command string:\n", command_string
s.send(command_string)

#from time import sleep
#sleep(10);

#import memcache
#mc = memcache.Client([(HOST, PORT)])
#for int in ints:
#    print "Checking %d" % int
#    if mc.get(str(int)) != str(int):
#        print "Incorrect value in the db: %s => %s" % (str(int), mc.get(str(int)))
#        import sys
#        sys.exit(-1)
