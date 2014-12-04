# Copyright 2010-2014 RethinkDB, all rights reserved.

__all__ = [
    'js', 'http', 'json', 'args', 'error', 'random', 'do', 'row', 'branch', 'map',
    'object', 'binary', 'uuid', 'type_of', 'info', 'range', 'literal', 'asc', 'desc',
    'table', 'db', 'db_config', 'db_create', 'db_drop', 'db_list', 'reconfigure',
    'table_config', 'table_create', 'table_drop', 'table_list', 'table_status', 'table_wait',
    'eq', 'ne', 'le', 'ge', 'lt', 'gt', 'any', 'all', 'and_', 'or_', 'not_',
    'add', 'sub', 'mul', 'div', 'mod',
    'time', 'iso8601', 'epoch_time', 'now', 'make_timezone',
    'monday', 'tuesday', 'wednesday', 'thursday', 'friday', 'saturday', 'sunday',
    'january', 'february', 'march', 'april', 'may', 'june',
    'july', 'august', 'september', 'october', 'november', 'december',
    'geojson', 'point', 'line', 'polygon', 'distance', 'intersects', 'circle'
]

from . import ast
from . import ql2_pb2 as p
import datetime

pTerm = p.Term.TermType

"""
All top level functions defined here are the starting points for RQL queries
"""
def json(*args):
    return ast.Json(*args)

def js(*args, **kwargs):
    return ast.JavaScript(*args, **kwargs)

def args(*args):
    return ast.Args(*args)

def http(url, **kwargs):
    return ast.Http(ast.func_wrap(url), **kwargs)

def error(*msg):
    return ast.UserError(*msg)

def random(*args, **kwargs):
    return ast.Random(*args, **kwargs)

def do(*args):
    return ast.FunCall(*args)

row = ast.ImplicitVar()

def table(*args, **kwargs):
    return ast.Table(*args, **kwargs)

def db(*args):
    return ast.DB(*args)

def db_create(*args):
    return ast.DbCreate(*args)

def db_drop(*args):
    return ast.DbDrop(*args)

def db_list(*args):
    return ast.DbList(*args)

def db_config(*args):
    return DbConfig(*args)

def table_create(*args, **kwargs):
    return ast.TableCreateTL(*args, **kwargs)

def table_drop(*args):
    return ast.TableDropTL(*args)

def table_list(*args):
    return ast.TableListTL(*args)

def table_config(*args):
    return ast.TableConfigTL(*args)

def table_status(*args):
    return ast.TableStatusTL(*args)

def table_wait(*args):
    return ast.TableWaitTL(*args)

def reconfigure(*args, **kwargs):
    return ast.ReconfigureTL(*args, **kwargs)

def branch(*args):
    return ast.Branch(*args)

def map(*args):
    if len(args) > 0:
        # `func_wrap` only the last argument
        return ast.Map(*(args[:-1] + (func_wrap(args[-1]), )))
    else:
        return ast.Map()

# orderBy orders

def asc(*args):
    return ast.Asc(*[ast.func_wrap(arg) for arg in args])

def desc(*args):
    return ast.Desc(*[ast.func_wrap(arg) for arg in args])

# math and logic

def eq(*args):
    return ast.Eq(*args)

def ne(*args):
    return ast.Ne(*args)

def lt(*args):
    return ast.Lt(*args)

def le(*args):
    return ast.Le(*args)

def gt(*args):
    return ast.Gt(*args)

def ge(*args):
    return ast.Ge(*args)

def add(*args):
    return ast.Add(*args)

def sub(*args):
    return ast.Sub(*args)

def mul(*args):
    return ast.Mul(*args)

def div(*args):
    return ast.Div(*args)

def mod(*args):
    return ast.Mod(*args)

def not_(*args):
    return ast.Not(*args)

def and_(*args):
    return ast.All(*args)

def or_(*args):
    return ast.Any(*args)

def all(*args):
    return ast.All(*args)

def any(*args):
    return ast.Any(*args)

def type_of(*args):
    return ast.TypeOf(*args)

def info(*args):
    return ast.Info(*args)

def binary(data):
    return ast.Binary(data)

def range(*args):
    return ast.Range(*args)

def time(*args):
    return ast.Time(*args)

def iso8601(*args, **kwargs):
    return ast.ISO8601(*args, **kwargs)

def epoch_time(*args):
    return ast.EpochTime(*args)

def now(*args):
    return ast.Now(*args)

class RqlTimeName(ast.RqlQuery):
    def compose(self, args, optargs):
        return 'r.'+self.st

# Time enum values
monday      = type('', (RqlTimeName,), {'tt':pTerm.MONDAY, 'st': 'monday'})()
tuesday     = type('', (RqlTimeName,), {'tt':pTerm.TUESDAY, 'st': 'tuesday'})()
wednesday   = type('', (RqlTimeName,), {'tt':pTerm.WEDNESDAY, 'st': 'wednesday'})()
thursday    = type('', (RqlTimeName,), {'tt':pTerm.THURSDAY, 'st': 'thursday'})()
friday      = type('', (RqlTimeName,), {'tt':pTerm.FRIDAY, 'st': 'friday'})()
saturday    = type('', (RqlTimeName,), {'tt':pTerm.SATURDAY, 'st': 'saturday'})()
sunday      = type('', (RqlTimeName,), {'tt':pTerm.SUNDAY, 'st': 'sunday'})()

january     = type('', (RqlTimeName,), {'tt':pTerm.JANUARY, 'st': 'january'})()
february    = type('', (RqlTimeName,), {'tt':pTerm.FEBRUARY, 'st': 'february'})()
march       = type('', (RqlTimeName,), {'tt': pTerm.MARCH, 'st': 'march'})()
april       = type('', (RqlTimeName,), {'tt': pTerm.APRIL, 'st': 'april'})()
may         = type('', (RqlTimeName,), {'tt': pTerm.MAY, 'st': 'may'})()
june        = type('', (RqlTimeName,), {'tt': pTerm.JUNE, 'st': 'june'})()
july        = type('', (RqlTimeName,), {'tt': pTerm.JULY, 'st': 'july'})()
august      = type('', (RqlTimeName,), {'tt': pTerm.AUGUST, 'st': 'august'})()
september   = type('', (RqlTimeName,), {'tt': pTerm.SEPTEMBER, 'st': 'september'})()
october     = type('', (RqlTimeName,), {'tt': pTerm.OCTOBER, 'st': 'october'})()
november    = type('', (RqlTimeName,), {'tt': pTerm.NOVEMBER, 'st': 'november'})()
december    = type('', (RqlTimeName,), {'tt': pTerm.DECEMBER, 'st': 'december'})()

def make_timezone(*args):
    return ast.RqlTzinfo(*args)

# Merge values
def literal(*args):
    return ast.Literal(*args)

def object(*args):
    return ast.Object(*args)

def uuid(*args):
    return ast.UUID(*args)

# Global geospatial operations
def geojson(*args):
    return ast.GeoJson(*args)

def point(*args):
    return ast.Point(*args)

def line(*args):
    return ast.Line(*args)

def polygon(*args):
    return ast.Polygon(*args)

def distance(*args, **kwargs):
    return ast.Distance(*args, **kwargs)

def intersects(*args):
    return ast.Intersects(*args)

def circle(*args, **kwargs):
    return ast.Circle(*args, **kwargs)
