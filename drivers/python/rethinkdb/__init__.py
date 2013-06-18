# This file includes all public facing Python API functions

# Set an environment variable telling the protobuf library
# to use the fast C++ based serializer implementation
# over the pure python one if it is available.
from os import environ
environ['PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION'] = 'cpp'

# Import the native extension defining the protobuf
# format to take advantage of even faster serialization
import pbcpp

from net import connect, Connection, Cursor
from query import js, error, do, row, table, db, db_create, db_drop, db_list, table_create, table_drop, table_list, branch, count, sum, avg, asc, desc, eq, ne, le, ge, lt, gt, any, all, add, sub, mul, div, mod, type_of, info
from errors import RqlError, RqlClientError, RqlCompileError, RqlRuntimeError, RqlDriverError
from ast import expr, RqlQuery
