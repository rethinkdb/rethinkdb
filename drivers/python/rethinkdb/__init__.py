# Copyright 2010-2014 RethinkDB, all rights reserved.

# Define this before importing so nothing can overwrite 'object'
class r(object):
    pass

from .net import *
from .query import *
from .errors import *
from .ast import *
from . import docs

for module in (net, query, ast, errors):
    for functionName in module.__all__:
        setattr(r, functionName, staticmethod(getattr(module, functionName)))
rethinkdb = r

# set the _r attribute to net.Connection
Connection._r = r

__all__ = ['r', 'rethinkdb'] + errors.__all__
