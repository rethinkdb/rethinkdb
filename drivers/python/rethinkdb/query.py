# Copyright 2010-2014 RethinkDB, all rights reserved.

__all__ = [
    'js', 'http', 'json', 'args', 'error', 'random', 'do', 'row', 'branch',
    'union', 'map', 'object', 'binary', 'uuid', 'type_of', 'info', 'range',
    'literal', 'asc', 'desc',
    'db', 'db_create', 'db_drop', 'db_list',
    'table', 'table_create', 'table_drop', 'table_list', 'grant',
    'group', 'reduce', 'count', 'sum', 'avg', 'min', 'max', 'distinct',
    'contains', 'eq', 'ne', 'le', 'ge', 'lt', 'gt', 'and_', 'or_', 'not_',
    'add', 'sub', 'mul', 'div', 'mod', 'floor', 'ceil', 'round',
    'time', 'iso8601', 'epoch_time', 'now', 'make_timezone',
    'monday', 'tuesday', 'wednesday', 'thursday', 'friday', 'saturday',
    'sunday',
    'january', 'february', 'march', 'april', 'may', 'june',
    'july', 'august', 'september', 'october', 'november', 'december',
    'minval', 'maxval',
    'geojson', 'point', 'line', 'polygon', 'distance', 'intersects', 'circle'
]

from . import ast
from . import ql2_pb2 as p

pTerm = p.Term.TermType


#
# All top level functions defined here are the starting points for RQL queries
#


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
    return ast.DbConfig(*args)


def table_create(*args, **kwargs):
    return ast.TableCreateTL(*args, **kwargs)


def table_drop(*args):
    return ast.TableDropTL(*args)


def table_list(*args):
    return ast.TableListTL(*args)


def grant(*args, **kwargs):
    return ast.GrantTL(*args, **kwargs)


def branch(*args):
    return ast.Branch(*args)


def union(*args):
    return ast.Union(*args)


def map(*args):
    if len(args) > 0:
        # `func_wrap` only the last argument
        return ast.Map(*(args[:-1] + (ast.func_wrap(args[-1]), )))
    else:
        return ast.Map()

# aggregation
def group(*args):
    return ast.Group(*[ast.func_wrap(arg) for arg in args])


def reduce(*args):
    return ast.Reduce(*[ast.func_wrap(arg) for arg in args])


def count(*args):
    return ast.Count(*[ast.func_wrap(arg) for arg in args])


def sum(*args):
    return ast.Sum(*[ast.func_wrap(arg) for arg in args])


def avg(*args):
    return ast.Avg(*[ast.func_wrap(arg) for arg in args])


def min(*args):
    return ast.Min(*[ast.func_wrap(arg) for arg in args])


def max(*args):
    return ast.Max(*[ast.func_wrap(arg) for arg in args])


def distinct(*args):
    return ast.Distinct(*[ast.func_wrap(arg) for arg in args])


def contains(*args):
    return ast.Contains(*[ast.func_wrap(arg) for arg in args])


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


def floor(*args):
    return ast.Floor(*args)


def ceil(*args):
    return ast.Ceil(*args)


def round(*args):
    return ast.Round(*args)


def not_(*args):
    return ast.Not(*args)


def and_(*args):
    return ast.And(*args)


def or_(*args):
    return ast.Or(*args)


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


class RqlConstant(ast.RqlQuery):
    def compose(self, args, optargs):
        return 'r.'+self.st

# Time enum values
monday      = type('monday', (RqlConstant,), {'tt':pTerm.MONDAY, 'st': 'monday'})()
tuesday     = type('tuesday', (RqlConstant,), {'tt':pTerm.TUESDAY, 'st': 'tuesday'})()
wednesday   = type('wednesday', (RqlConstant,), {'tt':pTerm.WEDNESDAY, 'st': 'wednesday'})()
thursday    = type('thursday', (RqlConstant,), {'tt':pTerm.THURSDAY, 'st': 'thursday'})()
friday      = type('friday', (RqlConstant,), {'tt':pTerm.FRIDAY, 'st': 'friday'})()
saturday    = type('saturday', (RqlConstant,), {'tt':pTerm.SATURDAY, 'st': 'saturday'})()
sunday      = type('sunday', (RqlConstant,), {'tt':pTerm.SUNDAY, 'st': 'sunday'})()

january     = type('january', (RqlConstant,), {'tt':pTerm.JANUARY, 'st': 'january'})()
february    = type('february', (RqlConstant,), {'tt':pTerm.FEBRUARY, 'st': 'february'})()
march       = type('march', (RqlConstant,), {'tt': pTerm.MARCH, 'st': 'march'})()
april       = type('april', (RqlConstant,), {'tt': pTerm.APRIL, 'st': 'april'})()
may         = type('may', (RqlConstant,), {'tt': pTerm.MAY, 'st': 'may'})()
june        = type('june', (RqlConstant,), {'tt': pTerm.JUNE, 'st': 'june'})()
july        = type('july', (RqlConstant,), {'tt': pTerm.JULY, 'st': 'july'})()
august      = type('august', (RqlConstant,), {'tt': pTerm.AUGUST, 'st': 'august'})()
september   = type('september', (RqlConstant,), {'tt': pTerm.SEPTEMBER, 'st': 'september'})()
october     = type('october', (RqlConstant,), {'tt': pTerm.OCTOBER, 'st': 'october'})()
november    = type('november', (RqlConstant,), {'tt': pTerm.NOVEMBER, 'st': 'november'})()
december    = type('december', (RqlConstant,), {'tt': pTerm.DECEMBER, 'st': 'december'})()

minval      = type('minval', (RqlConstant,), {'tt': pTerm.MINVAL, 'st': 'minval'})()
maxval      = type('maxval', (RqlConstant,), {'tt': pTerm.MAXVAL, 'st': 'maxval'})()


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
