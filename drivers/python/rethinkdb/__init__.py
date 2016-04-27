# Copyright 2010-2016 RethinkDB, all rights reserved.

from . import net, query, errors, ast, version

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
net.Connection._r = r

__version__ = version.version

__all__ = ['r', 'rethinkdb'] + errors.__all__
