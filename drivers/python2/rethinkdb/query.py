from ast import *
import ql2_pb2 as p

"""
All top level functions defined here are the starting points for RQL queries
"""

# This is both an external function and one used extensively
# internally to convert coerce python values to RQL types
def expr(val):
    '''
        Convert a Python primitive into a RQL primitive value
    '''
    if isinstance(val, RDBBase):
        return val
    elif isinstance(val, list):
        return MakeArray(*val)
    elif isinstance(val, dict):
        return MakeObj(**val)
    elif isinstance(val, types.LambdaType):
        return Func(val)
    else:
        return Datum(val)

def js(js_str):
    return JavaScript(js_str)

def error(msg):
    return UserError(msg)

def do(arg0, *args):
    args = [arg0]+[x for x in args]
    return FunCall(func_wrap(args[-1]), *args[:-1])
    
row = ImplicitVar()

def table(tbl_name, use_outdated=False):
    return Table(tbl_name, use_outdated=use_outdated)

def db(db_name):
    return DB(db_name)

def db_create(db_name):
    return DbCreate(db_name)

def db_drop(db_name):
    return DbDrop(db_name)

def db_list():
    return DbList()

def branch(predicate, true_branch, false_branch):
    return Branch(predicate, true_branch, false_branch)

count = {'COUNT': True}

def sum(attr):
    return {'SUM': attr}

def avg(attr):
    return {'AVG': attr}
