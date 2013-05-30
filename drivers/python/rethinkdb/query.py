from ast import *
import ql2_pb2 as p

"""
All top level functions defined here are the starting points for RQL queries
"""

def js(js_str, timeout=()):
    return JavaScript(js_str, timeout=timeout)

def error(*msg):
    return UserError(*msg)

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

def table_create(table_name, primary_key=(), datacenter=(), cache_size=(), hard_durability=()):
    return TableCreate(table_name, primary_key=primary_key, datacenter=datacenter, cache_size=cache_size, hard_durability=hard_durability)

def table_drop(table_name):
    return TableDrop(table_name)

def table_list():
    return TableList()

def branch(predicate, true_branch, false_branch):
    return Branch(predicate, true_branch, false_branch)

# groupBy reductions

count = {'COUNT': True}

def sum(attr):
    return {'SUM': attr}

def avg(attr):
    return {'AVG': attr}

# orderBy orders

def asc(attr):
    return Asc(attr)

def desc(attr):
    return Desc(attr)

# math and logic

def eq(*args):
    return Eq(*args)

def ne(*args):
    return Ne(*args)

def lt(*args):
    return Lt(*args)

def le(*args):
    return Le(*args)

def gt(*args):
    return Gt(*args)

def ge(*args):
    return Ge(*args)

def add(*args):
    return Add(*args)

def sub(*args):
    return Sub(*args)

def mul(*args):
    return Mul(*args)

def div(*args):
    return Div(*args)

def mod(a, b):
    return Mod(a, b)

def all(*args):
    return All(*args)

def any(*args):
    return Any(*args)

def type_of(val):
    return TypeOf(val)

def info(val):
    return Info(val)
