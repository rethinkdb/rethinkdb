from .ast import *
from . import ql2_pb2 as p
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
    return Asc(func_wrap(attr))

def desc(attr):
    return Desc(func_wrap(attr))

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

def not_(a):
    return Not(a)

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

def iso8601(string, default_timezone=()):
    return ISO8601(string, default_timezone=default_timezone)

def epoch_time(number):
    return EpochTime(number)

def now():
    return Now()

class RqlTimeName(RqlQuery):
    def compose(self, args, optargs):
        return 'r.'+self.st

# Time enum values
monday      = type('', (RqlTimeName,), {'tt':p.Term.MONDAY, 'st': 'monday'})()
tuesday     = type('', (RqlTimeName,), {'tt':p.Term.TUESDAY, 'st': 'tuesday'})()
wednesday   = type('', (RqlTimeName,), {'tt':p.Term.WEDNESDAY, 'st': 'wednesday'})()
thursday    = type('', (RqlTimeName,), {'tt':p.Term.THURSDAY, 'st': 'thursday'})()
friday      = type('', (RqlTimeName,), {'tt':p.Term.FRIDAY, 'st': 'friday'})()
saturday    = type('', (RqlTimeName,), {'tt':p.Term.SATURDAY, 'st': 'saturday'})()
sunday      = type('', (RqlTimeName,), {'tt':p.Term.SUNDAY, 'st': 'sunday'})()

january     = type('', (RqlTimeName,), {'tt':p.Term.JANUARY, 'st': 'january'})()
february    = type('', (RqlTimeName,), {'tt':p.Term.FEBRUARY, 'st': 'february'})()
march       = type('', (RqlTimeName,), {'tt': p.Term.MARCH, 'st': 'march'})()
april       = type('', (RqlTimeName,), {'tt': p.Term.APRIL, 'st': 'april'})()
may         = type('', (RqlTimeName,), {'tt': p.Term.MAY, 'st': 'may'})()
june        = type('', (RqlTimeName,), {'tt': p.Term.JUNE, 'st': 'june'})()
july        = type('', (RqlTimeName,), {'tt': p.Term.JULY, 'st': 'july'})()
august      = type('', (RqlTimeName,), {'tt': p.Term.AUGUST, 'st': 'august'})()
september   = type('', (RqlTimeName,), {'tt': p.Term.SEPTEMBER, 'st': 'september'})()
october     = type('', (RqlTimeName,), {'tt': p.Term.OCTOBER, 'st': 'october'})()
november    = type('', (RqlTimeName,), {'tt': p.Term.NOVEMBER, 'st': 'november'})()
december    = type('', (RqlTimeName,), {'tt': p.Term.DECEMBER, 'st': 'december'})()

def make_timezone(tzstring):
    return RqlTzinfo(tzstring)

# Merge values
def literal(val=()):
    if val:
        return Literal(val)
    else:
        return Literal()
