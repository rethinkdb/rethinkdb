#!/usr/bin/python

from redis import Redis
import random
import sys

elements = 1000

r = Redis('localhost', 6380, 0)
c = Redis('localhost', 6379, 0)

def aply(fun, *args):
    return (getattr(r, fun)('ss', *args)
           ,getattr(c, fun)('ss', *args))

aply('delete')

'''
for i in xrange(0,elements):
    data = random.random()
    if random.random() > 0.5:
        aply('lpush', data)
    else:
        aply('rpush', data)
'''

aply('zadd', 'bob', 1.0)
aply('zadd', 'bb', 9.0)
aply('zadd', 'bo', 4.0)
aply('zadd', 'obob', 5.0)

(rethink_range,redis_range) = aply('zrange', 0, 2, True, True)

if (rethink_range == redis_range): 
    print "OK"
else:
    print "Rethink", rethink_range
    print "Redis  ", redis_range
