import ql2_pb2 as p
import types
import sys
from threading import Lock
from errors import *
import repl # For the repl connection

# This is both an external function and one used extensively
# internally to convert coerce python values to RQL types
def expr(val):
    '''
        Convert a Python primitive into a RQL primitive value
    '''
    if isinstance(val, RqlQuery):
        return val
    elif isinstance(val, list):
        return MakeArray(*val)
    elif isinstance(val, dict):
        # MakeObj doesn't take the dict as a keyword args to avoid
        # conflicting with the `self` parameter.
        return MakeObj(val)
    elif callable(val):
        return Func(val)
    else:
        return Datum(val)

class RqlQuery(object):

    # Instantiate this AST node with the given pos and opt args
    def __init__(self, *args, **optargs):
        self.args = [expr(e) for e in args]

        self.optargs = {}
        for k in optargs.keys():
            if not isinstance(optargs[k], RqlQuery) and optargs[k] == ():
                continue
            self.optargs[k] = expr(optargs[k])

    # Send this query to the server to be executed
    def run(self, c=None, **global_opt_args):
        if not c:
            if repl.default_connection:
                c = repl.default_connection
            else:
                raise RqlDriverError("RqlQuery.run must be given a connection to run on.")

        return c._start(self, **global_opt_args)

    def __str__(self):
        qp = QueryPrinter(self)
        return qp.print_query()

    def __repr__(self):
        return "<RqlQuery instance: {0} >".format(str(self))

    # Compile this query to a binary protobuf buffer
    def build(self, term):
        term.type = self.tt

        for arg in self.args:
            arg.build(term.args.add())

        for k in self.optargs.keys():
            pair = term.optargs.add()
            pair.key = k
            self.optargs[k].build(pair.val)

    # The following are all operators and methods that operate on
    # Rql queries to build up more complex operations

    # Comparison operators
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

    # Numeric operators
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

    def has_fields(self, *attr):
        return HasFields(self, *attr)

    def with_fields(self, *attr):
        return WithFields(self, *attr)

    def keys(self):
        return Keys(self)

    # Polymorphic object/sequence operations
    def pluck(self, *attrs):
        return Pluck(self, *attrs)

    def without(self, *attrs):
        return Without(self, *attrs)

    def do(self, func):
        return FunCall(func_wrap(func), self)

    def default(self, handler):
        return Default(self, handler)

    def update(self, func, non_atomic=(), durability=()):
        return Update(self, func_wrap(func), non_atomic=non_atomic, durability=durability)

    def replace(self, func, non_atomic=(), durability=()):
        return Replace(self, func_wrap(func), non_atomic=non_atomic, durability=durability)

    def delete(self, durability=()):
        return Delete(self, durability=durability)

    # Rql type inspection
    def coerce_to(self, other_type):
        return CoerceTo(self, other_type)

    def type_of(self):
        return TypeOf(self)

    def merge(self, other):
        return Merge(self, other)

    def append(self, val):
        return Append(self, val)

    def prepend(self, val):
        return Prepend(self, val)

    def difference(self, val):
        return Difference(self, val)

    def set_insert(self, val):
        return SetInsert(self, val)

    def set_union(self, val):
        return SetUnion(self, val)

    def set_intersection(self, val):
        return SetIntersection(self, val)

    def set_difference(self, val):
        return SetDifference(self, val)

    # Operator used for get attr / nth / slice. Non-operator versions below
    # in cases of ambiguity
    def __getitem__(self, index):
        if isinstance(index, slice):
            return Slice(self, index.start or 0, index.stop or -1)
        elif isinstance(index, int):
            return Nth(self, index)
        elif isinstance(index, types.StringTypes):
            return GetAttr(self, index)

    def nth(self, index):
        return Nth(self, index)

    def match(self, pattern):
        return Match(self, pattern)

    def is_empty(self):
        return IsEmpty(self)

    def indexes_of(self, val):
        return IndexesOf(self,func_wrap(val))

    def slice(self, left=None, right=None):
        return Slice(self, left, right)

    def skip(self, index):
        return Skip(self, index)

    def limit(self, index):
        return Limit(self, index)

    def reduce(self, func, base=()):
        return Reduce(self, func_wrap(func), base=base)

    def map(self, func):
        return Map(self, func_wrap(func))

    def filter(self, func, default=()):
        return Filter(self, func_wrap(func), default=default)

    def concat_map(self, func):
        return ConcatMap(self, func_wrap(func))

    def order_by(self, *obs):
        return OrderBy(self, *obs)

    def between(self, left_bound=None, right_bound=None, index=()):
        return Between(self, left_bound, right_bound, index=index)

    def distinct(self):
        return Distinct(self)

    # NB: Can't overload __len__ because Python doesn't
    #     allow us to return a non-integer
    def count(self, filter=()):
        if filter == ():
            return Count(self)
        else:
            return Count(self, func_wrap(filter))

    def union(self, *others):
        return Union(self, *others)

    def inner_join(self, other, predicate):
        return InnerJoin(self, other, predicate)

    def outer_join(self, other, predicate):
        return OuterJoin(self, other, predicate)

    def eq_join(self, left_attr, other, index=()):
        return EqJoin(self, left_attr, other, index=index)

    def zip(self):
        return Zip(self)

    def grouped_map_reduce(self, grouping, mapping, data_collector, base=()):
        return GroupedMapReduce(self, func_wrap(grouping), func_wrap(mapping),
            func_wrap(data_collector), base=base)

    def group_by(self, arg1, arg2, *rest):
        args = [arg1, arg2] + list(rest)
        return GroupBy(self, list(args[:-1]), args[-1])

    def for_each(self, mapping):
        return ForEach(self, func_wrap(mapping))

    def info(self):
        return Info(self)

    # Array only operations
    def insert_at(self, index, value):
        return InsertAt(self, index, value)

    def splice_at(self, index, values):
        return SpliceAt(self, index, values)

    def delete_at(self, *indexes):
        return DeleteAt(self, *indexes);

    def change_at(self, index, value):
        return ChangeAt(self, index, value);

    def sample(self, count):
        return Sample(self, count)

# These classes define how nodes are printed by overloading `compose`

def needs_wrap(arg):
    return isinstance(arg, Datum) or isinstance(arg, MakeArray) or isinstance(arg, MakeObj)

class RqlBiOperQuery(RqlQuery):
    def compose(self, args, optargs):
        if needs_wrap(self.args[0]) and needs_wrap(self.args[1]):
            args[0] = T('r.expr(', args[0], ')')

        return T('(', args[0], ' ', self.st, ' ', args[1], ')')

class RqlTopLevelQuery(RqlQuery):
    def compose(self, args, optargs):
        args.extend([name+'='+optargs[name] for name in optargs.keys()])
        return T('r.', self.st, '(', T(*(args), intsp=', '), ')')

class RqlMethodQuery(RqlQuery):
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
class Datum(RqlQuery):
    args = []
    optargs = {}

    def __init__(self, val):
        self.data = val

    def build(self, term):
        term.type = p.Term.DATUM

        if self.data == None:
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
            raise RuntimeError("Cannot build a query from a {0}".format(type(term).__name__, term))

    def compose(self, args, optargs):
        return repr(self.data)

    @staticmethod
    def deconstruct(datum):
        if datum.type == p.Datum.R_NULL:
            return None
        elif datum.type == p.Datum.R_BOOL:
            return datum.r_bool
        elif datum.type == p.Datum.R_NUM:
            # Convert to an integer if we think maybe the user might think of this
            # number as an integer. I have been assured that this is a "temporary"
            # behavior change until RQL supports native integers.
            num = datum.r_num
            if num % 1 == 0:
                # Then we assume that in the user's data model this floating point
                # number is meant be an integer and "helpfully" convert types for them.
                num = int(num)
            return num
        elif datum.type == p.Datum.R_STR:
            return datum.r_str
        elif datum.type == p.Datum.R_ARRAY:
            return [Datum.deconstruct(e) for e in datum.r_array]
        elif datum.type == p.Datum.R_OBJECT:
            obj = {}
            for pair in datum.r_object:
                obj[pair.key] = Datum.deconstruct(pair.val)
            return obj
        else:
            raise RuntimeError("Unknown Datum type {0:d} encountered in response.".format(datum.type))

class MakeArray(RqlQuery):
    tt = p.Term.MAKE_ARRAY

    def compose(self, args, optargs):
        return T('[', T(*args, intsp=', '),']')

    def do(self, func):
        return FunCall(func_wrap(func), self)

class MakeObj(RqlQuery):
    tt = p.Term.MAKE_OBJ

    # We cannot inherit from RqlQuery because of potential conflicts with
    # the `self` parameter. This is not a problem for other RqlQuery sub-
    # classes unless we add a 'self' optional argument to one of them.
    def __init__(self, obj_dict):
        self.args = []

        self.optargs = {}
        for k in obj_dict.keys():
            if not isinstance(k, types.StringTypes):
                raise RqlDriverError("Object keys must be strings.");
            self.optargs[k] = expr(obj_dict[k])

    def compose(self, args, optargs):
        return T('{', T(*[T(repr(name), ': ', optargs[name]) for name in optargs.keys()], intsp=', '), '}')

class Var(RqlQuery):
    tt = p.Term.VAR

    def compose(self, args, optargs):
        return 'var_'+args[0]

class JavaScript(RqlTopLevelQuery):
    tt = p.Term.JAVASCRIPT
    st = "js"

class UserError(RqlTopLevelQuery):
    tt = p.Term.ERROR
    st = "error"

class Default(RqlQuery):
    tt = p.Term.DEFAULT
    st = "default"

class ImplicitVar(RqlQuery):
    tt = p.Term.IMPLICIT_VAR

    def compose(self, args, optargs):
        return 'r.row'

class Eq(RqlBiOperQuery):
    tt = p.Term.EQ
    st = "=="

class Ne(RqlBiOperQuery):
    tt = p.Term.NE
    st = "!="

class Lt(RqlBiOperQuery):
    tt = p.Term.LT
    st = "<"

class Le(RqlBiOperQuery):
    tt = p.Term.LE
    st = "<="

class Gt(RqlBiOperQuery):
    tt = p.Term.GT
    st = ">"

class Ge(RqlBiOperQuery):
    tt = p.Term.GE
    st = ">="

class Not(RqlQuery):
    tt = p.Term.NOT

    def compose(self, args, optargs):
        if isinstance(self.args[0], Datum):
            args[0] = T('r.expr(', args[0], ')')
        return T('(~', args[0], ')')

class Add(RqlBiOperQuery):
    tt = p.Term.ADD
    st = "+"

class Sub(RqlBiOperQuery):
    tt = p.Term.SUB
    st = "-"

class Mul(RqlBiOperQuery):
    tt = p.Term.MUL
    st = "*"

class Div(RqlBiOperQuery):
    tt = p.Term.DIV
    st = "/"

class Mod(RqlBiOperQuery):
    tt = p.Term.MOD
    st = "%"

class Append(RqlMethodQuery):
    tt = p.Term.APPEND
    st = "append"

class Prepend(RqlMethodQuery):
    tt = p.Term.PREPEND
    st = "prepend"

class Difference(RqlMethodQuery):
    tt = p.Term.DIFFERENCE
    st = "difference"

class SetInsert(RqlMethodQuery):
    tt = p.Term.SET_INSERT
    st = "set_insert"

class SetUnion(RqlMethodQuery):
    tt = p.Term.SET_UNION
    st = "set_union"

class SetIntersection(RqlMethodQuery):
    tt = p.Term.SET_INTERSECTION
    st = "set_intersection"

class SetDifference(RqlMethodQuery):
    tt = p.Term.SET_DIFFERENCE
    st = "set_difference"

class Slice(RqlQuery):
    tt = p.Term.SLICE

    def compose(self, args, optargs):
        return T(args[0], '[', args[1], ':', args[2], ']')

class Skip(RqlMethodQuery):
    tt = p.Term.SKIP
    st = 'skip'

class Limit(RqlMethodQuery):
    tt = p.Term.LIMIT
    st = 'limit'

class GetAttr(RqlQuery):
    tt = p.Term.GETATTR

    def compose(self, args, optargs):
        return T(args[0], '[', args[1], ']')

class Contains(RqlMethodQuery):
    tt = p.Term.CONTAINS
    st = 'contains'

class HasFields(RqlMethodQuery):
    tt = p.Term.HAS_FIELDS
    st = 'has_fields'

class WithFields(RqlMethodQuery):
    tt = p.Term.WITH_FIELDS
    st = 'with_fields'

class Keys(RqlMethodQuery):
    tt = p.Term.KEYS
    st = 'keys'

class Pluck(RqlMethodQuery):
    tt = p.Term.PLUCK
    st = 'pluck'

class Without(RqlMethodQuery):
    tt = p.Term.WITHOUT
    st = 'without'

class Merge(RqlMethodQuery):
    tt = p.Term.MERGE
    st = 'merge'

class Between(RqlMethodQuery):
    tt = p.Term.BETWEEN
    st = 'between'

class DB(RqlTopLevelQuery):
    tt = p.Term.DB
    st = 'db'

    def table_list(self):
        return TableList(self)

    def table_create(self, table_name, primary_key=(), datacenter=(), cache_size=(), durability=()):
        return TableCreate(self, table_name, primary_key=primary_key, datacenter=datacenter, cache_size=cache_size, durability=durability)

    def table_drop(self, table_name):
        return TableDrop(self, table_name)

    def table(self, table_name, use_outdated=()):
        return Table(self, table_name, use_outdated=use_outdated)

class FunCall(RqlQuery):
    tt = p.Term.FUNCALL

    def compose(self, args, optargs):
        if len(args) > 2:
            return T('r.do(', T(*(args[1:]), intsp=', '), ', ', args[0], ')')

        if isinstance(self.args[1], Datum):
            args[1] = T('r.expr(', args[1], ')')

        return T(args[1], '.do(', args[0], ')')

class Table(RqlQuery):
    tt = p.Term.TABLE
    st = 'table'

    def insert(self, records, upsert=(), durability=()):
        return Insert(self, records, upsert=upsert, durability=durability)

    def get(self, key):
        return Get(self, key)

    def get_all(self, key, index=()):
        return GetAll(self, key, index=index)

    def index_create(self, name, fundef=None):
        if fundef:
            return IndexCreate(self, name, func_wrap(fundef))
        else:
            return IndexCreate(self, name)

    def index_drop(self, name):
        return IndexDrop(self, name)

    def index_list(self):
        return IndexList(self)

    def compose(self, args, optargs):
        if isinstance(self.args[0], DB):
            return T(args[0], '.table(', args[1], ')')
        else:
            return T('r.table(', args[0], ')')

class Get(RqlMethodQuery):
    tt = p.Term.GET
    st = 'get'

class GetAll(RqlMethodQuery):
    tt = p.Term.GET_ALL
    st = 'get_all'

class Reduce(RqlMethodQuery):
    tt = p.Term.REDUCE
    st = 'reduce'

class Map(RqlMethodQuery):
    tt = p.Term.MAP
    st = 'map'

class Filter(RqlMethodQuery):
    tt = p.Term.FILTER
    st = 'filter'

class ConcatMap(RqlMethodQuery):
    tt = p.Term.CONCATMAP
    st = 'concat_map'

class OrderBy(RqlMethodQuery):
    tt = p.Term.ORDERBY
    st = 'order_by'

class Distinct(RqlMethodQuery):
    tt = p.Term.DISTINCT
    st = 'distinct'

class Count(RqlMethodQuery):
    tt = p.Term.COUNT
    st = 'count'

class Union(RqlMethodQuery):
    tt = p.Term.UNION
    st = 'union'

class Nth(RqlQuery):
    tt = p.Term.NTH

    def compose(self, args, optargs):
        return T(args[0], '[', args[1], ']')

class Match(RqlQuery):
    tt = p.Term.MATCH
    st = 'match'

class IndexesOf(RqlMethodQuery):
    tt = p.Term.INDEXES_OF
    st = 'indexes_of'

class IsEmpty(RqlMethodQuery):
    tt = p.Term.IS_EMPTY
    st = 'is_empty'

class IndexesOf(RqlMethodQuery):
    tt = p.Term.INDEXES_OF
    st = 'indexes_of'

class GroupedMapReduce(RqlMethodQuery):
    tt = p.Term.GROUPED_MAP_REDUCE
    st = 'grouped_map_reduce'

class GroupBy(RqlMethodQuery):
    tt = p.Term.GROUPBY
    st = 'group_by'

class InnerJoin(RqlMethodQuery):
    tt = p.Term.INNER_JOIN
    st = 'inner_join'

class OuterJoin(RqlMethodQuery):
    tt = p.Term.OUTER_JOIN
    st = 'outer_join'

class EqJoin(RqlMethodQuery):
    tt = p.Term.EQ_JOIN
    st = 'eq_join'

class Zip(RqlMethodQuery):
    tt = p.Term.ZIP
    st = 'zip'

class CoerceTo(RqlMethodQuery):
    tt = p.Term.COERCE_TO
    st = 'coerce_to'

class TypeOf(RqlMethodQuery):
    tt = p.Term.TYPEOF
    st = 'type_of'

class Update(RqlMethodQuery):
    tt = p.Term.UPDATE
    st = 'update'

class Delete(RqlMethodQuery):
    tt = p.Term.DELETE
    st = 'delete'

class Replace(RqlMethodQuery):
    tt = p.Term.REPLACE
    st = 'replace'

class Insert(RqlMethodQuery):
    tt = p.Term.INSERT
    st = 'insert'

class DbCreate(RqlTopLevelQuery):
    tt = p.Term.DB_CREATE
    st = "db_create"

class DbDrop(RqlTopLevelQuery):
    tt = p.Term.DB_DROP
    st = "db_drop"

class DbList(RqlTopLevelQuery):
    tt = p.Term.DB_LIST
    st = "db_list"

class TableCreate(RqlMethodQuery):
    tt = p.Term.TABLE_CREATE
    st = "table_create"

class TableCreateTL(RqlTopLevelQuery):
    tt = p.Term.TABLE_CREATE
    st = "table_create"

class TableDrop(RqlMethodQuery):
    tt = p.Term.TABLE_DROP
    st = "table_drop"

class TableDropTL(RqlTopLevelQuery):
    tt = p.Term.TABLE_DROP
    st = "table_drop"

class TableList(RqlMethodQuery):
    tt = p.Term.TABLE_LIST
    st = "table_list"

class TableListTL(RqlTopLevelQuery):
    tt = p.Term.TABLE_LIST
    st = "table_list"

class IndexCreate(RqlMethodQuery):
    tt = p.Term.INDEX_CREATE
    st = 'index_create'

class IndexDrop(RqlMethodQuery):
    tt = p.Term.INDEX_DROP
    st = 'index_drop'

class IndexList(RqlMethodQuery):
    tt = p.Term.INDEX_LIST
    st = 'index_list'

class Branch(RqlTopLevelQuery):
    tt = p.Term.BRANCH
    st = "branch"

class Any(RqlBiOperQuery):
    tt = p.Term.ANY
    st = "|"

class All(RqlBiOperQuery):
    tt = p.Term.ALL
    st = "&"

class ForEach(RqlMethodQuery):
    tt = p.Term.FOREACH
    st = 'for_each'

class Info(RqlMethodQuery):
    tt = p.Term.INFO
    st = 'info'

class InsertAt(RqlMethodQuery):
    tt = p.Term.INSERT_AT
    st = 'insert_at'

class SpliceAt(RqlMethodQuery):
    tt = p.Term.SPLICE_AT
    st = 'splice_at'

class DeleteAt(RqlMethodQuery):
    tt = p.Term.DELETE_AT
    st = 'delete_at'

class ChangeAt(RqlMethodQuery):
    tt = p.Term.CHANGE_AT
    st = 'change_at'

class Sample(RqlMethodQuery):
    tt = p.Term.SAMPLE
    st = 'sample'

# Called on arguments that should be functions
def func_wrap(val):
    val = expr(val)

    # Scan for IMPLICIT_VAR or JS
    def ivar_scan(node):
        if not isinstance(node, RqlQuery):
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

class Func(RqlQuery):
    tt = p.Term.FUNC
    lock = Lock()
    nextVarId = 1

    def __init__(self, lmbd):
        vrs = []
        vrids = []
        for i in xrange(lmbd.func_code.co_argcount):
            vrs.append(Var(Func.nextVarId))
            vrids.append(Func.nextVarId)
            Func.lock.acquire()
            Func.nextVarId += 1
            Func.lock.release()

        self.vrs = vrs
        self.args = [MakeArray(*vrids), expr(lmbd(*vrs))]
        self.optargs = {}

    def compose(self, args, optargs):
            return T('lambda ', T(*[v.compose([v.args[0].compose(None, None)], []) for v in self.vrs], intsp=', '), ': ', args[1])

class Asc(RqlTopLevelQuery):
    tt = p.Term.ASC
    st = 'asc'

class Desc(RqlTopLevelQuery):
    tt = p.Term.DESC
    st = 'desc'
