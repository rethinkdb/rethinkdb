# Copyright 2010-2014 RethinkDB, all rights reserved.

from .net import *
from .query import *
from .errors import *
from .ast import *
from . import docs

try:
    import __builtin__ as builtins # Python 2
except ImportError:
    import builtins # Python 3

# The builtins here defends against re-importing something obscuring `object`.
class r(builtins.object):
    pass

for module in (net, query, ast, errors):
    for functionName in module.__all__:
        setattr(r, functionName, staticmethod(getattr(module, functionName)))
rethinkdb = r

# set the _r attribute to net.Connection
Connection._r = r

__all__ = ['r', 'rethinkdb'] + errors.__all__
