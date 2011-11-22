from redis import Redis
import string
import random

def id_gen(len):
    return ''.join(random.choice(string.ascii_uppercase + string.ascii_lowercase) for x in range(len))

r = Redis('localhost', 6380, 0)
c = Redis('localhost', 6379, 0)

_key_ = id_gen(12)

def cmd(fun, *args):
    return (getattr(r, fun)(_key_, *args)
           ,getattr(c, fun)(_key_, *args))

