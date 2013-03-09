import ql2_pb2 as p
import types
import sys
from errors import *

class RDBBase():
    def run(self, c, **global_opt_args):
        return c._start(self, **global_opt_args)

    def __str__(self):
        qp = QueryPrinter(self)
        return qp.print_query()

    def __repr__(self):
        return "<RDBBase instance: %s >" % str(self)

class RDBOp(RDBBase):
    def __init__(self, *args, **optargs):
        self.args = [expr(e) for e in args]

        self.optargs = {}
        for k in optargs.keys():
            if optargs[k] is ():
                continue
            self.optargs[k] = expr(optargs[k])

    def build(self, term):
        term.type = self.tt

        for arg in self.args:
            arg.build(term.args.add())

        for k in self.optargs.keys():
            pair = term.optargs.add()
            pair.key = k
            self.optargs[k].build(pair.val)

class RDBValue(RDBOp):
    def __eq__(self, other):
        return Eq(self, other)

    def __ne__(self, other):
        return Ne(self, other)

    def __lt__(self, other):
        return Lt(self, other)

    def __le__(self, other):
        return Le(self, other)

    def __gt__(self, other):
        return Gt(self, other)

    def __ge__(self, other):
        return Ge(self, other)

    def __invert__(self):
        return Not(self)

    def __add__(self, other):
        return Add(self, other)

    def __radd__(self, other):
        return Add(other, self)

    def __sub__(self, other):
        return Sub(self, other)

    def __rsub__(self, other):
        return Sub(other, self)

    def __mul__(self, other):
        return Mul(self, other)

    def __rmul__(self, other):
        return Mul(other, self)

    def __div__(self, other):
        return Div(self, other)

    def __rdiv__(self, other):
        return Div(other, self)

    def __mod__(self, other):
        return Mod(self, other)

    def __rmod__(self, other):
        return Mod(other, self)

    def __and__(self, other):
        return All(self, other)

    def __rand__(self, other):
        return All(other, self)

    def __or__(self, other):
        return Any(self, other)

    def __ror__(self, other):
        return Any(other, self)

    # Non-operator versions of the above

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

    def mod(self, other):
        return Mod(self, other)

    # N.B. Cannot use 'in' operator because it must return a boolean
    def contains(self, *attr):
        return Contains(self, *attr)

    def pluck(self, *attrs):
        return Pluck(self, *attrs)

    def without(self, *attrs):
        return Without(self, *attrs)

    def do(self, func):
        return FunCall(func_wrap(func), self)

    def update(self, func, non_atomic=()):
        return Update(self, func_wrap(func), non_atomic=non_atomic)

    def replace(self, func, non_atomic=()):
        return Replace(self, func_wrap(func), non_atomic=non_atomic)

    def delete(self):
        return Delete(self)

    def coerce_to(self, other_type):
        return CoerceTo(self, other_type)

    def type_of(self):
        return TypeOf(self)

    def merge(self, other):
        return Merge(self, other)

    def append(self, val):
        return Append(self, val)

    def __getitem__(self, index):
        if isinstance(index, slice):
            # Because of a bug in Python 2 an open ended slice
            # sets stop to sys.maxint rather than None
            stop = index.stop
            if stop == sys.maxint or stop == None:
                stop = -1
            return Slice(self, index.start, stop)
        elif isinstance(index, int):
            return Nth(self, index)
        elif isinstance(index, types.StringTypes):
            return GetAttr(self, index)

    # Also define nth to add consistency and avoid the ambiguity of __getitem__
    def nth(self, index):
        return Nth(self, index)

    def slice(self, left=None, right=None):
        return Slice(self, left, right)

    def skip(self, index):
        return Skip(self, index)

    def limit(self, index):
        return Limit(self, index)

    def pluck(self, *attrs):
        return Pluck(self, *attrs)

    def without(self, *attrs):
        return Without(self, *attrs)

    def reduce(self, func, base=()):
        return Reduce(self, func_wrap(func), base=base)

    def map(self, func):
        return Map(self, func_wrap(func))

    def filter(self, func):
        return Filter(self, func_wrap(func))

    def concat_map(self, func):
        return ConcatMap(self, func_wrap(func))

    def order_by(self, *obs):
        return OrderBy(self, *obs)

    def between(self, left_bound=None, right_bound=None):
        # This is odd and inconsistent with the rest of the API. Blame a
        # poorly thought out spec.
        if left_bound is None:
            left_bound = ()
        if right_bound is None:
            right_bound = ()
        return Between(self, left_bound=left_bound, right_bound=right_bound)

    def distinct(self):
        return Distinct(self)

    # NB: Can't overload __len__ because Python doesn't
    #     allow us to return a non-integer
    def count(self):
        return Count(self)

    def union(self, *others):
        return Union(self, *others)

    def inner_join(self, other, predicate):
        return InnerJoin(self, other, predicate)

    def outer_join(self, other, predicate):
        return OuterJoin(self, other, predicate)

    def eq_join(self, left_attr, other):
        return EqJoin(self, left_attr, other)

    def zip(self):
        return Zip(self)

    def grouped_map_reduce(self, grouping, mapping, data_collector, base=()):
        return GroupedMapReduce(self, func_wrap(grouping), func_wrap(mapping),
            func_wrap(data_collector), base=base)

    def group_by(self, arg1, arg2, *rest):
        args = [arg1, arg2] + list(rest)
        return GroupBy(self, list(args[:-1]), args[-1])

    def delete(self):
        return Delete(self)

    def for_each(self, mapping):
        return ForEach(self, func_wrap(mapping))

### Representation classes

def needs_wrap(arg):
    return isinstance(arg, Datum) or isinstance(arg, MakeArray) or isinstance(arg, MakeObj)

class RDBBiOper:
    def compose(self, args, optargs):
        if needs_wrap(self.args[0]) and needs_wrap(self.args[1]):
            args[0] = T('r.expr(', args[0], ')')

        return T('(', args[0], ' ', self.st, ' ', args[1], ')')

class RDBTopFun:
    def compose(self, args, optargs):
        args.extend([repr(name)+'='+optargs[name] for name in optargs.keys()])
        return T('r.', self.st, '(', T(*(args), intsp=', '), ')')

class RDBMethod:
    def compose(self, args, optargs):
        if needs_wrap(self.args[0]):
            args[0] = T('r.expr(', args[0], ')')

        restargs = args[1:]
        restargs.extend([k+'='+v for k,v in optargs.items()])
        restargs = T(*restargs, intsp=', ')

        return T(args[0], '.', self.st, '(', restargs, ')')

# This class handles the conversion of RQL terminal types in both directions
# Going to the server though it does not support R_ARRAY or R_OBJECT as those
# are alternately handled by the MakeArray and MakeObject nodes. Why do this?
# MakeArray and MakeObject are more flexible, allowing us to construct array
# and object expressions from nested RQL expressions. Constructing pure
# R_ARRAYs and R_OBJECTs would require verifying that at all nested levels
# our arrays and objects are composed only of basic types.
class Datum(RDBValue):
    args = []
    optargs = {}

    def __init__(self, val):
        self.data = val

    def build(self, term):
        term.type = p.Term.DATUM

        if self.data is None:
            term.datum.type = p.Datum.R_NULL
        elif isinstance(self.data, bool):
            term.datum.type = p.Datum.R_BOOL
            term.datum.r_bool = self.data
        elif isinstance(self.data, int) or isinstance(self.data, float) or isinstance(self.data, long):
            term.datum.type = p.Datum.R_NUM
            term.datum.r_num = self.data
        elif isinstance(self.data, types.StringTypes):
            term.datum.type = p.Datum.R_STR
            term.datum.r_str = self.data
        else:
            raise RuntimeError("Cannot build a query from a %s" % type(term).__name__, term)

    def compose(self, args, optargs):
        return repr(self.data)

    @staticmethod
    def deconstruct(datum):
        if datum.type is p.Datum.R_NULL:
            return None
        elif datum.type is p.Datum.R_BOOL:
            return datum.r_bool
        elif datum.type is p.Datum.R_NUM:
            return datum.r_num
        elif datum.type is p.Datum.R_STR:
            return datum.r_str
        elif datum.type is p.Datum.R_ARRAY:
            return [Datum.deconstruct(e) for e in datum.r_array]
        elif datum.type is p.Datum.R_OBJECT:
            obj = {}
            for pair in datum.r_object:
                obj[pair.key] = Datum.deconstruct(pair.val)
            return obj
        else:
            raise RuntimeError("type not handled")

class MakeArray(RDBValue):
    tt = p.Term.MAKE_ARRAY

    def compose(self, args, optargs):
        return T('[', T(*args, intsp=', '),']')

    def do(self, func):
        return FunCall(func_wrap(func), self)

class MakeObj(RDBValue):
    tt = p.Term.MAKE_OBJ

    def compose(self, args, optargs):
        return T('{', T(*[T(repr(name), ': ', optargs[name]) for name in optargs.keys()], intsp=', '), '}')

class Var(RDBValue):
    tt = p.Term.VAR

    def compose(self, args, optargs):
        return 'var_'+args[0]

class JavaScript(RDBValue, RDBTopFun):
    tt = p.Term.JAVASCRIPT
    st = "js"

class UserError(RDBValue, RDBTopFun):
    tt = p.Term.ERROR
    st = "error"

class ImplicitVar(RDBValue):
    tt = p.Term.IMPLICIT_VAR

    def compose(self, args, optargs):
        return 'r.row'

class Eq(RDBValue, RDBBiOper):
    tt = p.Term.EQ
    st = "=="

class Ne(RDBValue, RDBBiOper):
    tt = p.Term.NE
    st = "!="

class Lt(RDBValue, RDBBiOper):
    tt = p.Term.LT
    st = "<"

class Le(RDBValue, RDBBiOper):
    tt = p.Term.LE
    st = "<="

class Gt(RDBValue, RDBBiOper):
    tt = p.Term.GT
    st = ">"

class Ge(RDBValue, RDBBiOper):
    tt = p.Term.GE
    st = ">="

class Not(RDBValue):
    tt = p.Term.NOT

    def compose(self, args, optargs):
        if isinstance(self.args[0], Datum):
            args[0] = T('r.expr(', args[0], ')')
        return T('(~', args[0], ')')

class Add(RDBValue, RDBBiOper):
    tt = p.Term.ADD
    st = "+"

class Sub(RDBValue, RDBBiOper):
    tt = p.Term.SUB
    st = "-"

class Mul(RDBValue, RDBBiOper):
    tt = p.Term.MUL
    st = "*"

class Div(RDBValue, RDBBiOper):
    tt = p.Term.DIV
    st = "/"

class Mod(RDBValue, RDBBiOper):
    tt = p.Term.MOD
    st = "%"

class Append(RDBValue, RDBMethod):
    tt = p.Term.APPEND
    st = "append"

class Slice(RDBValue):
    tt = p.Term.SLICE

    def compose(self, args, optargs):
        return T(args[0], '[', args[1], ':', args[2], ']')

class Skip(RDBValue, RDBMethod):
    tt = p.Term.SKIP
    st = 'skip'

class Limit(RDBValue, RDBMethod):
    tt = p.Term.LIMIT
    st = 'limit'

class GetAttr(RDBValue):
    tt = p.Term.GETATTR

    def compose(self, args, optargs):
        return T(args[0], '[', args[1], ']')

class Contains(RDBValue, RDBMethod):
    tt = p.Term.CONTAINS
    st = 'contains'

class Pluck(RDBValue, RDBMethod):
    tt = p.Term.PLUCK
    st = 'pluck'

class Without(RDBValue, RDBMethod):
    tt = p.Term.WITHOUT
    st = 'without'

class Merge(RDBValue, RDBMethod):
    tt = p.Term.MERGE
    st = 'merge'

class Between(RDBValue, RDBMethod):
    tt = p.Term.BETWEEN
    st = 'between'

class DB(RDBValue, RDBTopFun):
    tt = p.Term.DB
    st = 'db'

    def table_list(self):
        return TableList(self)

    def table_create(self, table_name, primary_key=(), datacenter=(), cache_size=()):
        return TableCreate(self, table_name, primary_key=primary_key, datacenter=datacenter, cache_size=cache_size)

    def table_drop(self, table_name):
        return TableDrop(self, table_name)

    def table(self, table_name, use_outdated=False):
        return Table(self, table_name, use_outdated=use_outdated)

class FunCall(RDBValue):
    tt = p.Term.FUNCALL

    def compose(self, args, optargs):
        if len(args) > 2:
            return T('r.do(', T(*(args[1:]), intsp=', '), ', ', args[0], ')')

        if isinstance(self.args[1], Datum):
            args[1] = T('r.expr(', args[1], ')')

        return T(args[1], '.do(', args[0], ')')

class Table(RDBValue):
    tt = p.Term.TABLE
    st = 'table'

    def insert(self, records, upsert=()):
        return Insert(self, records, upsert=upsert)

    def get(self, key):
        return Get(self, key)

    def compose(self, args, optargs):
        if isinstance(self.args[0], DB):
            return T(args[0], '.table(', args[1], ')')
        else:
            return T('r.table(', args[0], ')')

class Get(RDBValue, RDBMethod):
    tt = p.Term.GET
    st = 'get'

class Reduce(RDBValue, RDBMethod):
    tt = p.Term.REDUCE
    st = 'reduce'

class Map(RDBValue, RDBMethod):
    tt = p.Term.MAP
    st = 'map'

class Filter(RDBValue, RDBMethod):
    tt = p.Term.FILTER
    st = 'filter'

class ConcatMap(RDBValue, RDBMethod):
    tt = p.Term.CONCATMAP
    st = 'concat_map'

class OrderBy(RDBValue, RDBMethod):
    tt = p.Term.ORDERBY
    st = 'order_by'

class Distinct(RDBValue, RDBMethod):
    tt = p.Term.DISTINCT
    st = 'distinct'

class Count(RDBValue, RDBMethod):
    tt = p.Term.COUNT
    st = 'count'

class Union(RDBValue, RDBMethod):
    tt = p.Term.UNION
    st = 'union'

class Nth(RDBValue):
    tt = p.Term.NTH

    def compose(self, args, optargs):
        return T(args[0], '[', args[1], ']')

class GroupedMapReduce(RDBValue, RDBMethod):
    tt = p.Term.GROUPED_MAP_REDUCE
    st = 'grouped_map_reduce'

class GroupBy(RDBValue, RDBMethod):
    tt = p.Term.GROUPBY
    st = 'group_by'

class InnerJoin(RDBValue, RDBMethod):
    tt = p.Term.INNER_JOIN
    st = 'inner_join'

class OuterJoin(RDBValue, RDBMethod):
    tt = p.Term.OUTER_JOIN
    st = 'outer_join'

class EqJoin(RDBValue, RDBMethod):
    tt = p.Term.EQ_JOIN
    st = 'eq_join'

class Zip(RDBValue, RDBMethod):
    tt = p.Term.ZIP
    st = 'zip'

class CoerceTo(RDBValue, RDBMethod):
    tt = p.Term.COERCE_TO
    st = 'coerce_to'

class TypeOf(RDBValue, RDBMethod):
    tt = p.Term.TYPEOF
    st = 'type_of'

class Update(RDBValue, RDBMethod):
    tt = p.Term.UPDATE
    st = 'update'

class Delete(RDBValue, RDBMethod):
    tt = p.Term.DELETE
    st = 'delete'

class Replace(RDBValue, RDBMethod):
    tt = p.Term.REPLACE
    st = 'replace'

class Insert(RDBValue, RDBMethod):
    tt = p.Term.INSERT
    st = 'insert'

class DbCreate(RDBValue, RDBTopFun):
    tt = p.Term.DB_CREATE
    st = "db_create"

class DbDrop(RDBValue, RDBTopFun):
    tt = p.Term.DB_DROP
    st = "db_drop"

class DbList(RDBValue, RDBTopFun):
    tt = p.Term.DB_LIST
    st = "db_list"

class TableCreate(RDBValue, RDBMethod):
    tt = p.Term.TABLE_CREATE
    st = "table_create"

class TableDrop(RDBValue, RDBMethod):
    tt = p.Term.TABLE_DROP
    st = "table_drop"

class TableList(RDBValue, RDBMethod):
    tt = p.Term.TABLE_LIST
    st = "table_list"

class Branch(RDBValue, RDBTopFun):
    tt = p.Term.BRANCH
    st = "branch"

class Any(RDBValue, RDBBiOper):
    tt = p.Term.ANY
    st = "|"

class All(RDBValue, RDBBiOper):
    tt = p.Term.ALL
    st = "&"

class ForEach(RDBValue, RDBMethod):
    tt =p.Term.FOREACH
    st = 'for_each'

# Called on arguments that should be functions
def func_wrap(val):
    val = expr(val)

    # Scan for IMPLICIT_VAR or JS
    def ivar_scan(node):
        if not isinstance(node, RDBBase):
            return False

        if isinstance(node, ImplicitVar):
            return True
        if any([ivar_scan(arg) for arg in node.args]):
            return True
        if any([ivar_scan(arg) for k,arg in node.optargs.iteritems()]):
            return True
        return False

    if ivar_scan(val):
        return Func(lambda x: val)

    return val

class Func(RDBValue):
    tt = p.Term.FUNC
    nextVarId = 1

    def __init__(self, lmbd):
        vrs = []
        vrids = []
        for i in xrange(lmbd.func_code.co_argcount):
            vrs.append(Var(Func.nextVarId))
            vrids.append(Func.nextVarId)
            Func.nextVarId += 1

        self.vrs = vrs
        self.args = [MakeArray(*vrids), expr(lmbd(*vrs))]
        self.optargs = {}

    def compose(self, args, optargs):
            return T('lambda ', T(*[v.compose([v.args[0].compose(None, None)], []) for v in self.vrs], intsp=', '), ': ', args[1])

class Asc(RDBValue, RDBTopFun):
    tt = p.Term.ASC
    st = 'asc'

class Desc(RDBValue, RDBTopFun):
    tt = p.Term.DESC
    st = 'desc'

from query import expr
