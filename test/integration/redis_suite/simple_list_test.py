#!/usr/bin/python

from redis import Redis
import random

elements = 1000

r = Redis('localhost', 6380, 0)
c = Redis('localhost', 6379, 0)

def aply(fun, *args):
    return (getattr(r, fun)('l', *args)
           ,getattr(c, fun)('l', *args))

aply('delete')

for i in xrange(0,elements):
    data = random.random()
    if random.random() > 0.5:
        aply('lpush', data)
    else:
        aply('rpush', data)

(rethink_range,redis_range) = aply('lrange', 0, -30)

if (rethink_range == redis_range): 
    print "OK"
else:
    print "Rethink", rethink_range
    print "Redis  ", redis_range
