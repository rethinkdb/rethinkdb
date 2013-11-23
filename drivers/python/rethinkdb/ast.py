from . import ql2_pb2 as p
import types
import sys
import datetime
import numbers
import collections
import time
import re
import json as py_json
from threading import Lock
from .errors import *
from . import repl # For the repl connection

# This is both an external function and one used extensively
# internally to convert coerce python values to RQL types
def expr(val, nesting_depth=20):
    '''
        Convert a Python primitive into a RQL primitive value
    '''
    if nesting_depth <= 0:
        raise RqlDriverError("Nesting depth limit exceeded")

    if isinstance(val, RqlQuery):
        return val
    elif isinstance(val, datetime.datetime) or isinstance(val, datetime.date):
        if not hasattr(val, 'tzinfo') or not val.tzinfo:
            raise RqlDriverError("""Cannot convert %s to ReQL time object
            without timezone information. You can add timezone information with
            the third party module \"pytz\" or by constructing ReQL compatible
            timezone values with r.make_timezone(\"[+-]HH:MM\"). Alternatively,
            use one of ReQL's bultin time constructors, r.now, r.time, or r.iso8601.
            """ % (type(val).__name__))
        return ISO8601(val.isoformat())
    elif isinstance(val, list):
        val = [expr(v, nesting_depth - 1) for v in val]
        return MakeArray(*val)
    elif isinstance(val, dict):
        # MakeObj doesn't take the dict as a keyword args to avoid
        # conflicting with the `self` parameter.
        obj = {}
        for k in val.keys():
            obj[k] = expr(val[k], nesting_depth - 1)
        return MakeObj(obj)
    elif isinstance(val, collections.Callable):
        return Func(val)
    else:
        return Datum(val)

# Like expr but attempts to serialize as much of the value as JSON
# as possible.
def exprJSON(val, nesting_depth=20):
    if nesting_depth <= 0:
        raise RqlDriverError("Nesting depth limit exceeded")

    if isinstance(val, RqlQuery):
        return val
    elif isJSON(val, nesting_depth):
        return Json(py_json.dumps(val))
    elif isinstance(val, dict):
        copy = val.copy()
        for k,v in copy.items():
            copy[k] = exprJSON(v, nesting_depth)
        return MakeObj(copy)
    elif isinstance(val, list):
        copy = []
        for v in val:
            copy.append(exprJSON(v, nesting_depth))
        return MakeArray(*copy)
    else:
        # Default to datum serialization
        return expr(val, nesting_depth - 1)

def isJSON(val, nesting_depth=20):
    if nesting_depth <= 0:
        raise RqlDriverError("Nesting depth limit exceeded")

    if isinstance(val, RqlQuery):
        return False
    elif isinstance(val, dict):
        return all([isinstance(k, types.StringTypes) and isJSON(v, nesting_depth - 1) for k,v in val.items()])
    elif isinstance(val, list):
        return all([isJSON(v, nesting_depth - 1) for v in val])
    elif isinstance(val, (int, float, str, unicode, bool)):
        return True
    else:
        return False

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
        return "<RqlQuery instance: %s >" % str(self)

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

    def not_(self):
        return Not(self)

    # N.B. Cannot use 'in' operator because it must return a boolean
    def contains(self, *attr):
        return Contains(self, *map(func_wrap, attr))

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

    def update(self, func, non_atomic=(), durability=(), return_vals=()):
        return Update(self, func_wrap(func), non_atomic=non_atomic,
                      durability=durability, return_vals=return_vals)

    def replace(self, func, non_atomic=(), durability=(), return_vals=()):
        return Replace(self, func_wrap(func), non_atomic=non_atomic,
                       durability=durability, return_vals=return_vals)

    def delete(self, durability=(), return_vals=()):
        return Delete(self, durability=durability, return_vals=return_vals)

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
            if index.stop:
                return Slice(self, index.start or 0, index.stop)
            else:
                return Slice(self, index.start or 0, -1, right_bound='closed')
        elif isinstance(index, int):
            return Nth(self, index)
        elif isinstance(index, types.StringTypes):
            return GetField(self, index)

    def __iter__(self):
        raise RqlDriverError(
                "__iter__ called on an RqlQuery object.\n"+
                "To iterate over the results of a query, call run first.\n"+
                "To iterate inside a query, use map or for_each.")

    def nth(self, index):
        return Nth(self, index)

    def match(self, pattern):
        return Match(self, pattern)

    def is_empty(self):
        return IsEmpty(self)

    def indexes_of(self, val):
        return IndexesOf(self,func_wrap(val))

    def slice(self, left, right, left_bound=(), right_bound=()):
        return Slice(self, left, right, left_bound=left_bound, right_bound=right_bound)

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

    def order_by(self, *obs, **kwargs):
        obs = [ob if isinstance(ob, Asc) or isinstance(ob, Desc) else func_wrap(ob) for ob in obs]
        return OrderBy(self, *obs, **kwargs)

    def between(self, left=None, right=None, left_bound=(), right_bound=(), index=()):
        return Between(self, left, right, left_bound=left_bound, right_bound=right_bound, index=index)

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

    ## Time support

    def to_iso8601(self):
        return ToISO8601(self)

    def to_epoch_time(self):
        return ToEpochTime(self)

    def during(self, t2, t3, left_bound=(), right_bound=()):
        return During(self, t2, t3, left_bound=left_bound, right_bound=right_bound)

    def date(self):
        return Date(self)

    def time_of_day(self):
        return TimeOfDay(self)

    def timezone(self):
        return Timezone(self)

    def year(self):
        return Year(self)

    def month(self):
        return Month(self)

    def day(self):
        return Day(self)

    def day_of_week(self):
        return DayOfWeek(self)

    def day_of_year(self):
        return DayOfYear(self)

    def hours(self):
        return Hours(self)

    def minutes(self):
        return Minutes(self)

    def seconds(self):
        return Seconds(self)

    def in_timezone(self, tzstr):
        return InTimezone(self, tzstr)

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
        args.extend([T(k, '=', v) for k,v in optargs.items()])
        return T('r.', self.st, '(', T(*(args), intsp=', '), ')')

class RqlMethodQuery(RqlQuery):
    def compose(self, args, optargs):
        if needs_wrap(self.args[0]):
            args[0] = T('r.expr(', args[0], ')')

        restargs = args[1:]
        restargs.extend([T(k, '=', v) for k,v in optargs.items()])
        restargs = T(*restargs, intsp=', ')

        return T(args[0], '.', self.st, '(', restargs, ')')

class RqlTzinfo(datetime.tzinfo):

    def __init__(self, offsetstr):
        hours, minutes = map(int, offsetstr.split(':'))

        self.offsetstr = offsetstr
        self.delta = datetime.timedelta(hours=hours, minutes=minutes)

    def __copy__(self):
        return RqlTzinfo(self.offsetstr)

    def __deepcopy__(self, memo):
        return RqlTzinfo(self.offsetstr)

    def utcoffset(self, dt):
        return self.delta

    def tzname(self, dt):
        return self.offsetstr

    def dst(self, dt):
        return datetime.timedelta(0)

def reql_type_time_to_datetime(obj):
    if not 'epoch_time' in obj:
        raise RqlDriverError('pseudo-type TIME object %s does not have expected field "epoch_time".' % py_json.dumps(obj))

    if 'timezone' in obj:
        return datetime.datetime.fromtimestamp(obj['epoch_time'], RqlTzinfo(obj['timezone']))
    else:
        return datetime.datetime.utcfromtimestamp(obj['epoch_time'])

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
        elif isinstance(self.data, numbers.Real):
            term.datum.type = p.Datum.R_NUM
            term.datum.r_num = self.data
        elif isinstance(self.data, types.StringTypes):
            term.datum.type = p.Datum.R_STR
            term.datum.r_str = self.data
        else:
            raise RqlDriverError("Cannot build a query from a %s" % type(self.data).__name__)

    def compose(self, args, optargs):
        return repr(self.data)

    @staticmethod
    def deconstruct(datum, time_format='native'):
        d_type = datum.type
        if d_type == p.Datum.R_JSON:
            return py_json.loads(datum.r_str)
        elif d_type == p.Datum.R_OBJECT:
            obj = { }
            for pair in datum.r_object:
                obj[pair.key] = Datum.deconstruct(pair.val, time_format)

            # Thanks to "pseudo-types" we can't yet be quite sure if this object is meant to
            # be an object or something else. We need a second layer of type switching, this
            # time on an obfuscated field "$reql_type$" rather than the datum type field we
            # already switched on.
            reql_type = obj.get('$reql_type$')
            if reql_type is not None:
                if reql_type == 'TIME':
                    if time_format == 'native':
                        # Convert to native python datetime object
                        return reql_type_time_to_datetime(obj)
                    elif time_format == 'raw':
                        # Just return the raw `{'$reql_type':...}` dict
                        return obj
                    else:
                        raise RqlDriverError("Unknown time_format run option \"%s\"." % time_format)
                else:
                    raise RqlDriverError("Unknown pseudo-type %s" % reql_type)

            return obj
        elif d_type == p.Datum.R_ARRAY:
            array = datum.r_array
            return [Datum.deconstruct(e, time_format) for e in array]
        elif d_type == p.Datum.R_STR:
            return datum.r_str
        elif d_type == p.Datum.R_NUM:
            # Convert to an integer if we think maybe the user might think of this
            # number as an integer. I have been assured that this is a "temporary"
            # behavior change until RQL supports native integers.
            num = datum.r_num
            if num % 1 == 0:
                # Then we assume that in the user's data model this floating point
                # number is meant be an integer and "helpfully" convert types for them.
                num = int(num)
            return num
        elif d_type == p.Datum.R_BOOL:
            return datum.r_bool
        elif d_type == p.Datum.R_NULL:
            return None
        else:
            raise RuntimeError("Unknown Datum type %d encountered in response." % datum.type)

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

class Default(RqlMethodQuery):
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

class GetField(RqlQuery):
    tt = p.Term.GET_FIELD

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

    def insert(self, records, upsert=(), durability=(), return_vals=()):
        return Insert(self, exprJSON(records), upsert=upsert,
                      durability=durability, return_vals=return_vals)

    def get(self, key):
        return Get(self, key)

    def get_all(self, *keys, **kwargs):
        return GetAll(self, *keys, **kwargs)

    def index_create(self, name, fundef=(), multi=()):
        args = [self, name] + ([func_wrap(fundef)] if fundef else [])
        kwargs = {"multi" : multi} if multi else {}
        return IndexCreate(*args, **kwargs)

    def index_drop(self, name):
        return IndexDrop(self, name)

    def index_list(self):
        return IndexList(self)

    def index_status(self, *indexes):
        return IndexStatus(self, *indexes)

    def index_wait(self, *indexes):
        return IndexWait(self, *indexes)

    def sync(self):
        return Sync(self)

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

class Match(RqlMethodQuery):
    tt = p.Term.MATCH
    st = 'match'

class IndexesOf(RqlMethodQuery):
    tt = p.Term.INDEXES_OF
    st = 'indexes_of'

class IsEmpty(RqlMethodQuery):
    tt = p.Term.IS_EMPTY
    st = 'is_empty'

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

class IndexStatus(RqlMethodQuery):
    tt = p.Term.INDEX_STATUS
    st = 'index_status'

class IndexWait(RqlMethodQuery):
    tt = p.Term.INDEX_WAIT
    st = 'index_wait'

class Sync(RqlMethodQuery):
    tt = p.Term.SYNC
    st = 'sync'

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

class Json(RqlTopLevelQuery):
    tt = p.Term.JSON
    st = 'json'

class ToISO8601(RqlMethodQuery):
    tt = p.Term.TO_ISO8601
    st = 'to_iso8601'

class During(RqlMethodQuery):
    tt = p.Term.DURING
    st = 'during'

class Date(RqlMethodQuery):
    tt = p.Term.DATE
    st = 'date'

class TimeOfDay(RqlMethodQuery):
    tt = p.Term.TIME_OF_DAY
    st = 'time_of_day'

class Timezone(RqlMethodQuery):
    tt = p.Term.TIMEZONE
    st = 'timezone'

class Year(RqlMethodQuery):
    tt = p.Term.YEAR
    st = 'year'

class Month(RqlMethodQuery):
    tt = p.Term.MONTH
    st = 'month'

class Day(RqlMethodQuery):
    tt = p.Term.DAY
    st = 'day'

class DayOfWeek(RqlMethodQuery):
    tt = p.Term.DAY_OF_WEEK
    st = 'day_of_week'

class DayOfYear(RqlMethodQuery):
    tt = p.Term.DAY_OF_YEAR
    st = 'day_of_year'

class Hours(RqlMethodQuery):
    tt = p.Term.HOURS
    st = 'hours'

class Minutes(RqlMethodQuery):
    tt = p.Term.MINUTES
    st = 'minutes'

class Seconds(RqlMethodQuery):
    tt = p.Term.SECONDS
    st = 'seconds'

class Time(RqlTopLevelQuery):
    tt = p.Term.TIME
    st = 'time'

class ISO8601(RqlTopLevelQuery):
    tt = p.Term.ISO8601
    st = 'iso8601'

class EpochTime(RqlTopLevelQuery):
    tt = p.Term.EPOCH_TIME
    st = 'epoch_time'

class Now(RqlTopLevelQuery):
    tt = p.Term.NOW
    st = 'now'

class InTimezone(RqlMethodQuery):
    tt = p.Term.IN_TIMEZONE
    st = 'in_timezone'

class ToEpochTime(RqlMethodQuery):
    tt = p.Term.TO_EPOCH_TIME
    st = 'to_epoch_time'

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
        if any([ivar_scan(arg) for k,arg in node.optargs.items()]):
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
        for i in range(lmbd.func_code.co_argcount):
            Func.lock.acquire()
            var_id = Func.nextVarId
            Func.nextVarId += 1
            Func.lock.release()
            vrs.append(Var(var_id))
            vrids.append(var_id)

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

class Literal(RqlTopLevelQuery):
    tt = p.Term.LITERAL
    st = 'literal'
