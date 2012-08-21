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

__all__ = ['connect', 'expr', 'table', 'R', 'js', 'let', 'fn', 'if_then_else', 'ExecutionError', 'BadQueryError']

from query import *
from net import connect
from errors import *

# Package initialization
