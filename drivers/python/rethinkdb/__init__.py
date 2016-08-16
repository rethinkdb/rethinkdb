# Copyright 2010-2016 RethinkDB, all rights reserved.

from . import version, _dump, _export, _import, _index_rebuild, _restore
from .ast import *
from .errors import *
from .net import *
from .query import *

__all__ = ['r', 'rethinkdb'] + errors.__all__
__version__ = version.version

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
r._dump          = _dump
r._export        = _export
r._import        = _import
r._index_rebuild = _index_rebuild
r._restore       = _restore
rethinkdb = r

# set the _r attribute to net.Connection
net.Connection._r = r
