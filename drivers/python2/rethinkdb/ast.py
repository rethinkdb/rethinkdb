import ql2_pb2 as p
import types
import sys
from errors import *

class RDBBase():
    def run(self, c):
        return c._start(self)

class RDBValue(RDBBase):
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

    def __getitem__(self, key):
        return GetAttr(self, key)

    def __and__(self, other):
        return All(self, other)

    def __rand__(self, other):
        return All(other, self)

    def __or__(self, other):
        return Any(self, other)

    def __ror__(self, other):
        return Any(other, self)

    # N.B. Cannot use 'in' operator because it must return a boolean
    def contains(self, *attr):
        return Contains(self, *attr)

    def pluck(self, *attrs):
        return Pluck(self, *attrs)

    def without(self, *attrs):
        return Without(self, *attrs)

    def merge(self, other):
        return Merge(self, other)

    def do(self, func):
        return FunCall(Func(func), self)

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

class RDBSequence(RDBBase):
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
        return Reduce(self, Func(func), base=base)

    def map(self, func):
        return Map(self, Func(func))

    def filter(self, func):
        return Filter(self, Func(func))

    def concat_map(self, func):
        return ConcatMap(self, Func(func))

    def order_by(self, *obs):
        return OrderBy(self, *obs)

    def between(self, left_bound=(), right_bound=()):
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

    def eq_join(self, other, left_attr):
        return EqJoin(self, other, left_attr)

    def grouped_map_reduce(self, grouping, mapping, reduction):
        return GroupedMapReduce(self, grouping, mapping, reduction)

    def update(self, mapping):
        return Update(self, mapping)

    def delete(self):
        return Delete(self)

    def replace(self, mapping):
        return Replace(self, mapping)

    def for_each(self, mapping):
        return ForEach(self, mapping)
    
class RDBValOp(RDBValue, RDBOp):
    pass

class RDBSeqOp(RDBSequence, RDBOp):
    pass

class RDBAnyOp(RDBValue, RDBSequence, RDBOp):
    pass

### Representation classes

class RDBBiOper:
    def compose(self, args, optargs):
        return T('(', args[0], ' ', self.st, ' ', args[1], ')')

class RDBTopFun:
    def compose(self, args, optargs):
        args.extend([repr(name)+'='+optargs[name] for name in optargs.keys()])
        return T('r.', self.st, '(', T(*(args), intsp=', '), ')')

class RDBMethod:
    def compose(self, args, optargs):
        return T(args[0], '.', self.st, '(', T(*args[1:], intsp=', '), ')')

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
        term.type = p.Term2.DATUM

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
            raise RuntimeError("type not handled")

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

class MakeArray(RDBSeqOp):
    tt = p.Term2.MAKE_ARRAY

    def compose(self, args, optargs):
        return T('[', T(*args, intsp=', '),']')

    def do(self, func):
        return FunCall(Func(func), self)

class MakeObj(RDBValOp):
    tt = p.Term2.MAKE_OBJ

    def compose(self, args, optargs):
        return T('{', T(*[T(repr(name), ': ', optargs[name]) for name in optargs.keys()], intsp=', '), '}')

class Var(RDBAnyOp):
    tt = p.Term2.VAR

    def compose(self, args, optargs):
        return 'var_'+args[0]

class JavaScript(RDBValOp, RDBTopFun):
    tt = p.Term2.JAVASCRIPT
    st = "js"

class UserError(RDBOp, RDBTopFun):
    tt = p.Term2.ERROR
    st = "error"

class ImplicitVar(RDBAnyOp):
    tt = p.Term2.IMPLICIT_VAR

    def compose(self, args, optargs):
        return 'r.row'

class Eq(RDBValOp, RDBBiOper):
    tt = p.Term2.EQ
    st = "=="

class Ne(RDBValOp, RDBBiOper):
    tt = p.Term2.NE
    st = "!="

class Lt(RDBValOp, RDBBiOper):
    tt = p.Term2.LT
    st = "<="

class Le(RDBValOp, RDBBiOper):
    tt = p.Term2.LE
    st = "<="

class Gt(RDBValOp, RDBBiOper):
    tt = p.Term2.GT
    st = ">"

class Ge(RDBValOp, RDBBiOper):
    tt = p.Term2.GE
    st = ">="

class Not(RDBValOp, RDBMethod):
    tt = p.Term2.NOT
    st = "not"

class Add(RDBValOp, RDBBiOper):
    tt = p.Term2.ADD
    st = "+"

class Sub(RDBValOp, RDBBiOper):
    tt = p.Term2.SUB
    st = "-"

class Mul(RDBValOp, RDBBiOper):
    tt = p.Term2.MUL
    st = "*"

class Div(RDBValOp, RDBBiOper):
    tt = p.Term2.DIV
    st = "/"

class Mod(RDBValOp, RDBBiOper):
    tt = p.Term2.MOD
    st = "%"

class Append(RDBSeqOp, RDBMethod):
    tt = p.Term2.APPEND
    st = "append"

class Slice(RDBSeqOp):
    tt = p.Term2.SLICE

    def compose(self, args, optargs):
        return T(args[0], '[', args[1], ':', args[2], ']')

class Skip(RDBSeqOp, RDBMethod):
    tt = p.Term2.SKIP
    st = 'skip'

class Limit(RDBSeqOp, RDBMethod):
    tt = p.Term2.LIMIT
    st = 'limit'

class GetAttr(RDBValOp):
    tt = p.Term2.GETATTR

    def compose(self, args, optargs):
        return T(args[0], '[', args[1], ']')

class Contains(RDBValOp, RDBMethod):
    tt = p.Term2.CONTAINS
    st = 'contains'

class Pluck(RDBValOp, RDBMethod):
    tt = p.Term2.PLUCK
    st = 'pluck'

class Without(RDBValOp, RDBMethod):
    tt = p.Term2.WITHOUT
    st = 'without'

class Merge(RDBValOp, RDBMethod):
    tt = p.Term2.MERGE
    st = 'merge'

class Between(RDBSeqOp, RDBMethod):
    tt = p.Term2.BETWEEN
    st = 'between'

class DB(RDBOp, RDBTopFun):
    tt = p.Term2.DB
    st = 'db'

    def table_list(self):
        return TableList(self)

    def table_create(self, table_name):
        return TableCreate(self, table_name)

    def table_drop(self, table_name):
        return TableDrop(self, table_name)

    def table(self, table_name, use_outdated=False):
        return Table(self, table_name, use_outdated=use_outdated)

class FunCall(RDBAnyOp):
    tt = p.Term2.FUNCALL

    def compose(self, args, optargs):
        return T(args[1], '.do(', args[0], ')')

class Table(RDBSeqOp, RDBMethod):
    tt = p.Term2.TABLE
    st = 'table'

    def insert(self, records):
        if not isinstance(records, list):
            records = [records]
        return Insert(self, records)

    def get(self, key):
        return Get(self, key)

    def compose(self, args, optargs):
        return T(args[1], '.table(', args[0], ')')

class Get(RDBValOp, RDBMethod):
    tt = p.Term2.GET
    st = 'get'

class Reduce(RDBSeqOp, RDBMethod):
    tt = p.Term2.REDUCE
    st = 'reduce'

class Map(RDBSeqOp, RDBMethod):
    tt = p.Term2.MAP
    st = 'map'

class Filter(RDBSeqOp, RDBMethod):
    tt = p.Term2.FILTER
    st = 'filter'

class ConcatMap(RDBSeqOp, RDBMethod):
    tt = p.Term2.CONCATMAP
    st = 'concat_map'

class OrderBy(RDBSeqOp, RDBMethod):
    tt = p.Term2.ORDERBY
    st = 'order_by'

class Distinct(RDBSeqOp, RDBMethod):
    tt = p.Term2.DISTINCT
    st = 'distinct'

class Count(RDBValOp, RDBMethod):
    tt = p.Term2.COUNT
    st = 'count'

class Union(RDBSeqOp, RDBMethod):
    tt = p.Term2.UNION
    st = 'union'

class Nth(RDBValOp):
    tt = p.Term2.NTH

    def compose(self, args, optargs):
        return T(args[0], '[', args[1], ']')

class GroupedMapReduce(RDBValOp, RDBMethod):
    tt = p.Term2.GROUPED_MAP_REDUCE
    st = 'grouped_map_reduce'

class InnerJoin(RDBSeqOp, RDBMethod):
    tt = p.Term2.INNER_JOIN
    st = 'inner_join'

class OuterJoin(RDBSeqOp, RDBMethod):
    tt = p.Term2.OUTER_JOIN
    st = 'outer_join'

class EqJoin(RDBSeqOp, RDBMethod):
    tt = p.Term2.EQ_JOIN
    st = 'eq_join'

class Update(RDBOp, RDBMethod):
    tt = p.Term2.UPDATE
    st = 'update'

class Delete(RDBOp, RDBMethod):
    tt = p.Term2.DELETE
    st = 'delete'

class Replace(RDBOp, RDBMethod):
    tt = p.Term2.REPLACE
    st = 'replace'

class Insert(RDBOp, RDBMethod):
    tt = p.Term2.INSERT
    st = 'insert'

class DbCreate(RDBOp, RDBTopFun):
    tt = p.Term2.DB_CREATE
    st = "db_create"

class DbDrop(RDBOp, RDBTopFun):
    tt = p.Term2.DB_DROP
    st = "db_drop"

class DbList(RDBSeqOp, RDBTopFun):
    tt = p.Term2.DB_LIST
    st = "db_list"

class TableCreate(RDBOp, RDBMethod):
    tt = p.Term2.TABLE_CREATE
    st = "table_create"

class TableDrop(RDBOp, RDBMethod):
    tt = p.Term2.TABLE_DROP
    st = "table_drop"

class TableList(RDBSeqOp, RDBMethod):
    tt = p.Term2.TABLE_LIST
    st = "table_list"

class Branch(RDBAnyOp, RDBTopFun):
    tt = p.Term2.BRANCH
    st = "branch"

class Any(RDBValOp, RDBBiOper):
    tt = p.Term2.ANY
    st = "|"

class All(RDBValOp, RDBBiOper):
    tt = p.Term2.ALL
    st = "&"

class ForEach(RDBOp, RDBMethod):
    tt =p.Term2.FOREACH
    st = 'for_each'

class Func(RDBOp):
    tt = p.Term2.FUNC
    nextVarId = 1

    def __init__(self, lmbd):
        if isinstance(lmbd, types.FunctionType):
            vrs = []
            vrids = []
            for i in xrange(lmbd.func_code.co_argcount):
                vrs.append(Var(Func.nextVarId))
                vrids.append(Func.nextVarId)
                Func.nextVarId += 1

            self.vrs = vrs
            self.args = [MakeArray(*vrids), expr(lmbd(*vrs))]
        else:
            self.args = [MakeArray(), expr(lmbd)]
        
        self.optargs = {}

    def compose(self, args, optargs):
        return T('lambda ', T(*[v.compose([v.args[0].compose(None, None)], []) for v in self.vrs], intsp=', '), ': ', args[1])

from query import expr
