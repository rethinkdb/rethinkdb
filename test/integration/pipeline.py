#!/usr/bin/python
HOST = 'localhost'
PORT = 11213
nInts = 250

from time import sleep
import socket
import random

#split a list into a random number of lists of random size
#"I am a string" -> ["I a", "m a s", "strin", "g"]
def rand_split(string, avg_sub_string_size):
    nsub_strings = random.randint(1, (len(string) * 2) / avg_sub_string_size)
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

strings = rand_split(command_string, 40)
for string in strings:
    print string
    print "----"
    s.send(string)
    sleep(.05)

s.close()

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
