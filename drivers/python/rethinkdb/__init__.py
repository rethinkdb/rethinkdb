# Copyright 2010-2014 RethinkDB, all rights reserved.
# This file includes all public facing Python API functions

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

__all__ = ['r', 'rethinkdb'] + errors.__all__
