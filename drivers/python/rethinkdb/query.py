from ast import *
import ql2_pb2 as p
import datetime

"""
All top level functions defined here are the starting points for RQL queries
"""
def json(json_str):
    return Json(json_str)

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

def table_create(table_name, primary_key=(), datacenter=(), cache_size=(), durability=()):
    return TableCreateTL(table_name, primary_key=primary_key, datacenter=datacenter, cache_size=cache_size, durability=durability)

def table_drop(table_name):
    return TableDropTL(table_name)

def table_list():
    return TableListTL()

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

def time(*args):
    return Time(*args)

def iso8601(string):
    return ISO8601(string)

def epoch_time(number):
    return EpochTime(number)

def now():
    return Now()

# Time enum values
monday      = type('', (RqlTopLevelQuery,), {'tt':p.Term.MONDAY})()
tuesday     = type('', (RqlTopLevelQuery,), {'tt':p.Term.TUESDAY})()
wednesday   = type('', (RqlTopLevelQuery,), {'tt':p.Term.WEDNESDAY})()
thursday    = type('', (RqlTopLevelQuery,), {'tt':p.Term.THURSDAY})()
friday      = type('', (RqlTopLevelQuery,), {'tt':p.Term.FRIDAY})()
saturday    = type('', (RqlTopLevelQuery,), {'tt':p.Term.SATURDAY})()
sunday      = type('', (RqlTopLevelQuery,), {'tt':p.Term.SUNDAY})()

january     = type('', (RqlTopLevelQuery,), {'tt':p.Term.JANUARY})()
february    = type('', (RqlTopLevelQuery,), {'tt':p.Term.FEBRUARY})()
march       = type('', (RqlTopLevelQuery,), {'tt': p.Term.MARCH})()
april       = type('', (RqlTopLevelQuery,), {'tt': p.Term.APRIL})()
may         = type('', (RqlTopLevelQuery,), {'tt': p.Term.MAY})()
june        = type('', (RqlTopLevelQuery,), {'tt': p.Term.JUNE})()
july        = type('', (RqlTopLevelQuery,), {'tt': p.Term.JULY})()
august      = type('', (RqlTopLevelQuery,), {'tt': p.Term.AUGUST})()
september   = type('', (RqlTopLevelQuery,), {'tt': p.Term.SEPTEMBER})()
october     = type('', (RqlTopLevelQuery,), {'tt': p.Term.OCTOBER})()
november    = type('', (RqlTopLevelQuery,), {'tt': p.Term.NOVEMBER})()
december    = type('', (RqlTopLevelQuery,), {'tt': p.Term.DECEMBER})()

# Merge values
def literal(val=()):
    if val:
        return Literal(val)
    else:
        return Literal()
