#!/usr/bin/python

from testRedis import cmd
import random

elements = 1000

cmd('delete')

for i in xrange(0,elements):
    data = random.random()
    if random.random() > 0.5:
        cmd('lpush', data)
    else:
        cmd('rpush', data)

(rethink_range,redis_range) = cmd('lrange', 0, -30)

if (rethink_range == redis_range): 
    print "OK"
else:
    print "Rethink", rethink_range
    print "Redis  ", redis_range
