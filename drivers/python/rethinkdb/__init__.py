# Copyright 2010-2012 RethinkDB, all rights reserved.
"""This package implements ReQL - a domain specific language for
generation of RethinkDB queries embedded into Python. ReQL can be used
to query and update the database. It is designed to be very expressive
(i.e. to support advanced operations such as subqueries and table
joins), as well as to be user-friendly and easy to use.

ReQL is inspired by jQuery and SQLAlchemy. The jQuery style of
programming allows queries to be chained to form more complex queries
in an intuitive way. Following SQLAlchemy conventions, as many of
Python's facilities as possible have been overloaded in order to make
expressing queries friendly and convenient."""

__all__ = [
    'connect', 'last_connection',
    'db_create', 'db_drop', 'db_list', 'db', 'table', 'error',
    'expr', 'r', 'union', 'js', 'let', 'letvar', 'branch',
    'asc', 'desc',
    'ExecutionError', 'BadQueryError']

from query import *
from net import connect, last_connection, ExecutionError, BadQueryError

import sys


class RDBShortcut(object):
    """Get the value of a variable or attribute.

    Filter and map bind the current element to the implicit variable.
    To access the implicit variable (i.e. 'current' row), use the
    following:

    >>> r['@']


    To access an attribute of the implicit variable, pass the attribute name.

    >>> r['name']
    >>> r['@']['name']


    See information on scoping rules for more details.

    >>> table('users').filter(r['age'] == 30) # access attribute age from the implicit row variable
    >>> table('users').filter(r['address']['city') == 'Mountain View') # access subattribute city
                                                                       # of attribute address from
                                                                       # the implicit row variable
    >>> table('users').filter(lambda row: row['age') == 30)) # access attribute age from the
                                                             # variable 'row'
    """
    def __getitem__(self, key):
        if key == "@":
            expr = JSONExpression(internal.ImplicitVar())
        else:
            expr = JSONExpression(internal.ImplicitAttr(key))
        return expr

r = RDBShortcut()
for fun in dir(sys.modules[__name__]):
    setattr(r, fun, getattr(sys.modules[__name__], fun))

# Package initialization
