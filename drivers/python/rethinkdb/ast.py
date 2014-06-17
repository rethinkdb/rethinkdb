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
from . import ql2_pb2 as p
from ._compat import dict_items, str_types, range_

pTerm = p.Term.TermType

# This is both an external function and one used extensively
# internally to convert coerce python values to RQL types
def expr(val, nesting_depth=20):
    '''
        Convert a Python primitive into a RQL primitive value
    '''
    if not isinstance(nesting_depth, int):
        raise RqlDriverError("Second argument to `r.expr` must be a number.")

    if nesting_depth <= 0:
        raise RqlDriverError("Nesting depth limit exceeded")

    if isinstance(val, RqlQuery):
        return val
    elif isinstance(val, list):
        val = [expr(v, nesting_depth - 1) for v in val]
        return MakeArray(*val)
    elif isinstance(val, dict):
        # MakeObj doesn't take the dict as a keyword args to avoid
        # conflicting with the `self` parameter.
        obj = {}
        for (k,v) in dict_items(val):
            obj[k] = expr(v, nesting_depth - 1)
        return MakeObj(obj)
    elif isinstance(val, collections.Callable):
        return Func(val)
    elif isinstance(val, datetime.datetime) or isinstance(val, datetime.date):
        if not hasattr(val, 'tzinfo') or not val.tzinfo:
            raise RqlDriverError("""Cannot convert %s to ReQL time object
            without timezone information. You can add timezone information with
            the third party module \"pytz\" or by constructing ReQL compatible
            timezone values with r.make_timezone(\"[+-]HH:MM\"). Alternatively,
            use one of ReQL's bultin time constructors, r.now, r.time, or r.iso8601.
            """ % (type(val).__name__))
        return ISO8601(val.isoformat())
    else:
        return Datum(val)

class RqlQuery(object):

    # Instantiate this AST node with the given pos and opt args
    def __init__(self, *args, **optargs):
        self.args = [expr(e) for e in args]

        self.optargs = {}
        for (k,v) in dict_items(optargs):
            if not isinstance(v, RqlQuery) and v == ():
                continue
            self.optargs[k] = expr(v)

    # Send this query to the server to be executed
    def run(self, c=None, **global_optargs):
        if not c:
            if repl.default_connection:
                c = repl.default_connection
            else:
                raise RqlDriverError("RqlQuery.run must be given a connection to run on.")

        return c._start(self, **global_optargs)

    def __str__(self):
        qp = QueryPrinter(self)
        return qp.print_query()

    def __repr__(self):
        return "<RqlQuery instance: %s >" % str(self)

    # Compile this query to a json-serializable object
    def build(self):
        res = [self.tt, [arg.build() for arg in self.args]]
        if len(self.optargs) > 0:
            res.append(dict((k, v.build()) for (k,v) in dict_items(self.optargs)))
        return res

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

    def __truediv__(self, other):
        return Div(self, other)

    def __rtruediv__(self, other):
        return Div(other, self)

    def __mod__(self, other):
        return Mod(self, other)

    def __rmod__(self, other):
        return Mod(other, self)

    def __and__(self, other):
        query = All(self, other)
        query.set_infix()
        return query

    def __rand__(self, other):
        query = All(other, self)
        query.set_infix()
        return query

    def __or__(self, other):
        query = Any(self, other)
        query.set_infix()
        return query

    def __ror__(self, other):
        query = Any(other, self)
        query.set_infix()
        return query

    # Non-operator versions of the above

    def eq(self, *args):
        return Eq(self, *args)

    def ne(self, *args):
        return Ne(self, *args)

    def lt(self, *args):
        return Lt(self, *args)

    def le(self, *args):
        return Le(self, *args)

    def gt(self, *args):
        return Gt(self, *args)

    def ge(self, *args):
        return Ge(self, *args)

    def add(self, *args):
        return Add(self, *args)

    def sub(self, *args):
        return Sub(self, *args)

    def mul(self, *args):
        return Mul(self, *args)

    def div(self, *args):
        return Div(self, *args)

    def mod(self, *args):
        return Mod(self, *args)

    def and_(self, *args):
        return All(self, *args)

    def or_(self, *args):
        return Any(self, *args)

    def not_(self, *args):
        return Not(self, *args)

    # N.B. Cannot use 'in' operator because it must return a boolean
    def contains(self, *args):
        return Contains(self, *[func_wrap(arg) for arg in args])

    def has_fields(self, *args):
        return HasFields(self, *args)

    def with_fields(self, *args):
        return WithFields(self, *args)

    def keys(self, *args):
        return Keys(self, *args)

    def changes(self, *args):
        return Changes(self, *args)

    # Polymorphic object/sequence operations
    def pluck(self, *args):
        return Pluck(self, *args)

    def without(self, *args):
        return Without(self, *args)

    def do(self, *args):
        return FunCall(self, *args)

    def default(self, *args):
        return Default(self, *args)

    def update(self, *args, **kwargs):
        kwargs.setdefault('non_atomic', ())
        kwargs.setdefault('durability', ())
        kwargs.setdefault('return_vals', ())
        return Update(self, *[func_wrap(arg) for arg in args], **kwargs)

    def replace(self, *args, **kwargs):
        kwargs.setdefault('non_atomic', ())
        kwargs.setdefault('durability', ())
        kwargs.setdefault('return_vals', ())
        return Replace(self, *[func_wrap(arg) for arg in args], **kwargs)

    def delete(self, *args, **kwargs):
        kwargs.setdefault('durability', ())
        kwargs.setdefault('return_vals', ())
        return Delete(self, *args, **kwargs)

    # Rql type inspection
    def coerce_to(self, *args):
        return CoerceTo(self, *args)

    def ungroup(self, *args):
        return Ungroup(self, *args)

    def type_of(self, *args):
        return TypeOf(self, *args)

    def merge(self, *args):
        return Merge(self, *[func_wrap(arg) for arg in args])

    def append(self, *args):
        return Append(self, *args)

    def prepend(self, *args):
        return Prepend(self, *args)

    def difference(self, *args):
        return Difference(self, *args)

    def set_insert(self, *args):
        return SetInsert(self, *args)

    def set_union(self, *args):
        return SetUnion(self, *args)

    def set_intersection(self, *args):
        return SetIntersection(self, *args)

    def set_difference(self, *args):
        return SetDifference(self, *args)

    # Operator used for get attr / nth / slice. Non-operator versions below
    # in cases of ambiguity
    def __getitem__(self, index):
        if isinstance(index, slice):
            if index.stop:
                return Slice(self, index.start or 0, index.stop, bracket_operator=True)
            else:
                return Slice(self, index.start or 0, -1, right_bound='closed', bracket_operator=True)
        elif isinstance(index, int):
            return Nth(self, index, bracket_operator=True)
        elif isinstance(index, str_types):
            return GetField(self, index, bracket_operator=True)
        elif isinstance(index, RqlQuery):
            raise RqlDriverError(
                "Bracket operator called with a ReQL expression parameter.\n"+
                "Dynamic typing is not supported in this syntax,\n"+
                "use `.nth`, `.slice`, or `.get_field` instead.")
        else:
            raise RqlDriverError(
                "bracket operator called with an unsupported parameter type: %s.%s" %
                (index.__class__.__module__, index.__class__.__name__))

    def __iter__(*args, **kwargs):
        raise RqlDriverError(
                "__iter__ called on an RqlQuery object.\n"+
                "To iterate over the results of a query, call run first.\n"+
                "To iterate inside a query, use map or for_each.")

    def get_field(self, *args):
        return GetField(self, *args)

    def nth(self, *args):
        return Nth(self, *args)

    def match(self, *args):
        return Match(self, *args)

    def split(self, *args):
        return Split(self, *args)

    def upcase(self, *args):
        return Upcase(self, *args)

    def downcase(self, *args):
        return Downcase(self, *args)

    def is_empty(self, *args):
        return IsEmpty(self, *args)

    def indexes_of(self, *args):
        return IndexesOf(self, *[func_wrap(arg) for arg in args])

    def slice(self, *args, **kwargs):
        return Slice(self, *args, **kwargs)

    def skip(self, *args):
        return Skip(self, *args)

    def limit(self, *args):
        return Limit(self, *args)

    def reduce(self, *args):
        return Reduce(self, *[func_wrap(arg) for arg in args])

    def sum(self, *args):
        return Sum(self, *[func_wrap(arg) for arg in args])

    def avg(self, *args):
        return Avg(self, *[func_wrap(arg) for arg in args])

    def min(self, *args):
        return Min(self, *[func_wrap(arg) for arg in args])

    def max(self, *args):
        return Max(self, *[func_wrap(arg) for arg in args])

    def map(self, *args):
        return Map(self, *[func_wrap(arg) for arg in args])

    def filter(self, *args, **kwargs):
        kwargs.setdefault('default', ())
        return Filter(self, *[func_wrap(arg) for arg in args], **kwargs)

    def concat_map(self, *args):
        return ConcatMap(self, *[func_wrap(arg) for arg in args])

    def order_by(self, *args, **kwargs):
        args = [arg if isinstance(arg, Asc) or isinstance(arg, Desc) else func_wrap(arg) for arg in args]
        return OrderBy(self, *args, **kwargs)

    def between(self, *args, **kwargs):
        kwargs.setdefault('left_bound', ())
        kwargs.setdefault('right_bound', ())
        kwargs.setdefault('index', ())
        return Between(self, *args, **kwargs)

    def distinct(self, *args):
        return Distinct(self, *args)

    # NB: Can't overload __len__ because Python doesn't
    #     allow us to return a non-integer
    def count(self, *args):
        return Count(self, *[func_wrap(arg) for arg in args])

    def union(self, *args):
        return Union(self, *args)

    def inner_join(self, *args):
        return InnerJoin(self, *args)

    def outer_join(self, *args):
        return OuterJoin(self, *args)

    def eq_join(self, *args, **kwargs):
        kwargs.setdefault('index', ())
        return EqJoin(self, *[func_wrap(arg) for arg in args], **kwargs)

    def zip(self, *args):
        return Zip(self, *args)

    def group(self, *args, **kwargs):
        return Group(self, *[func_wrap(arg) for arg in args], **kwargs)

    def for_each(self, *args):
        return ForEach(self, *[func_wrap(arg) for arg in args])

    def info(self, *args):
        return Info(self, *args)

    # Array only operations
    def insert_at(self, *args):
        return InsertAt(self, *args)

    def splice_at(self, *args):
        return SpliceAt(self, *args)

    def delete_at(self, *args):
        return DeleteAt(self, *args);

    def change_at(self, *args):
        return ChangeAt(self, *args);

    def sample(self, *args):
        return Sample(self, *args)

    ## Time support

    def to_iso8601(self, *args):
        return ToISO8601(self, *args)

    def to_epoch_time(self, *args):
        return ToEpochTime(self, *args)

    def during(self, *args, **kwargs):
        kwargs.setdefault('left_bound', ())
        kwargs.setdefault('right_bound', ())
        return During(self, *args, **kwargs)

    def date(self, *args):
        return Date(self, *args)

    def time_of_day(self, *args):
        return TimeOfDay(self, *args)

    def timezone(self, *args):
        return Timezone(self, *args)

    def year(self, *args):
        return Year(self, *args)

    def month(self, *args):
        return Month(self, *args)

    def day(self, *args):
        return Day(self, *args)

    def day_of_week(self, *args):
        return DayOfWeek(self, *args)

    def day_of_year(self, *args):
        return DayOfYear(self, *args)

    def hours(self, *args):
        return Hours(self, *args)

    def minutes(self, *args):
        return Minutes(self, *args)

    def seconds(self, *args):
        return Seconds(self, *args)

    def in_timezone(self, *args):
        return InTimezone(self, *args)

# These classes define how nodes are printed by overloading `compose`

def needs_wrap(arg):
    return isinstance(arg, Datum) or isinstance(arg, MakeArray) or isinstance(arg, MakeObj)

class RqlBoolOperQuery(RqlQuery):
    def __init__(self, *args, **optargs):
        self.infix = False
        RqlQuery.__init__(self, *args, **optargs)

    def set_infix(self):
        self.infix = True

    def compose(self, args, optargs):
        t_args = [T('r.expr(', args[i], ')') if needs_wrap(self.args[i]) else args[i] for i in range_(len(args))]

        if self.infix:
            return T('(', T(*t_args, intsp=[' ', self.st_infix, ' ']), ')')
        else:
            return T('r.', self.st, '(', T(*t_args, intsp=', '), ')')

class RqlBiOperQuery(RqlQuery):
    def compose(self, args, optargs):
        t_args = [T('r.expr(', args[i], ')') if needs_wrap(self.args[i]) else args[i] for i in range_(len(args))]
        return T('(', T(*t_args, intsp=[' ', self.st, ' ']), ')')

class RqlBiCompareOperQuery(RqlBiOperQuery):
    def __init__(self, *args, **optargs):
        RqlBiOperQuery.__init__(self, *args, **optargs)

        for arg in args:
            try:
                if arg.infix:
                    err = "Calling '%s' on result of infix bitwise operator:\n" + \
                          "%s.\n" + \
                          "This is almost always a precedence error.\n" + \
                          "Note that `a < b | b < c` <==> `a < (b | b) < c`.\n" + \
                          "If you really want this behavior, use `.or_` or `.and_` instead."
                    raise RqlDriverError(err % (self.st, QueryPrinter(self).print_query()))
            except AttributeError:
                pass # No infix attribute, so not possible to be an infix bool operator

class RqlTopLevelQuery(RqlQuery):
    def compose(self, args, optargs):
        args.extend([T(k, '=', v) for (k,v) in dict_items(optargs)])
        return T('r.', self.st, '(', T(*(args), intsp=', '), ')')

class RqlMethodQuery(RqlQuery):
    def compose(self, args, optargs):
        if len(args) == 0:
            return T('r.', self.st, '()')

        if needs_wrap(self.args[0]):
            args[0] = T('r.expr(', args[0], ')')

        restargs = args[1:]
        restargs.extend([T(k, '=', v) for (k,v) in dict_items(optargs)])
        restargs = T(*restargs, intsp=', ')

        return T(args[0], '.', self.st, '(', restargs, ')')

class RqlBracketQuery(RqlMethodQuery):
    def __init__(self, *args, **optargs):
        if 'bracket_operator' in optargs:
            self.bracket_operator = optargs['bracket_operator']
            del optargs['bracket_operator']
        else:
            self.bracket_operator = False

        RqlMethodQuery.__init__(self, *args, **optargs)

    def compose(self, args, optargs):
        if self.bracket_operator:
            if needs_wrap(self.args[0]):
                args[0] = T('r.expr(', args[0], ')')
            return T(args[0], '[', T(*args[1:], intsp=[',']), ']')
        else:
            return RqlMethodQuery.compose(self, args, optargs)

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

# Python only allows immutable built-in types to be hashed, such as for keys in a dict
# This means we can't use lists or dicts as keys in grouped data objects, so we convert
# them to tuples and frozensets, respectively.
# This may make it a little harder for users to work with converted grouped data, unless
# they do a simple iteration over the result
def recursively_make_hashable(obj):
    if isinstance(obj, list):
        return tuple([recursively_make_hashable(i) for i in obj])
    elif isinstance(obj, dict):
        return frozenset([(k, recursively_make_hashable(v)) for (k,v) in dict_items(obj)])
    return obj

def reql_type_grouped_data_to_object(obj):
    if not 'data' in obj:
        raise RqlDriverError('pseudo-type GROUPED_DATA object %s does not have the expected field "data".' % py_json.dumps(obj))
    return dict([(recursively_make_hashable(k),v) for (k,v) in obj['data']])

def convert_pseudotype(obj, format_opts):
    reql_type = obj.get('$reql_type$')
    if reql_type is not None:
        if reql_type == 'TIME':
            time_format = format_opts.get('time_format')
            if time_format is None or time_format == 'native':
                # Convert to native python datetime object
                return reql_type_time_to_datetime(obj)
            elif time_format != 'raw':
                raise RqlDriverError("Unknown time_format run option \"%s\"." % time_format)
        elif reql_type == 'GROUPED_DATA':
            group_format = format_opts.get('group_format')
            if group_format is None or group_format == 'native':
                return reql_type_grouped_data_to_object(obj)
            elif group_format != 'raw':
                raise RqlDriverError("Unknown group_format run option \"%s\"." % group_format)
        else:
            raise RqlDriverError("Unknown pseudo-type %s" % reql_type)
    # If there was no pseudotype, or the time format is raw, return the original object
    return obj

def recursively_convert_pseudotypes(obj, format_opts):
    if isinstance(obj, dict):
        for (key, value) in dict_items(obj):
            obj[key] = recursively_convert_pseudotypes(value, format_opts)
        obj = convert_pseudotype(obj, format_opts)
    elif isinstance(obj, list):
        for i in range_(len(obj)):
            obj[i] = recursively_convert_pseudotypes(obj[i], format_opts)
    return obj

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

    def build(self):
        return self.data

    def compose(self, args, optargs):
        return repr(self.data)

class MakeArray(RqlQuery):
    tt = pTerm.MAKE_ARRAY

    def compose(self, args, optargs):
        return T('[', T(*args, intsp=', '),']')

    def do(self, *args):
        return FunCall(self, *args)

class MakeObj(RqlQuery):
    tt = pTerm.MAKE_OBJ

    # We cannot inherit from RqlQuery because of potential conflicts with
    # the `self` parameter. This is not a problem for other RqlQuery sub-
    # classes unless we add a 'self' optional argument to one of them.
    def __init__(self, obj_dict):
        self.args = []

        self.optargs = {}
        for (k,v) in dict_items(obj_dict):
            if not isinstance(k, str_types):
                raise RqlDriverError("Object keys must be strings.");
            self.optargs[k] = expr(v)

    def build(self):
        res = { }
        for (k,v) in dict_items(self.optargs):
            k = k.build() if isinstance(k, RqlQuery) else k
            res[k] = v.build() if isinstance(v, RqlQuery) else v
        return res

    def compose(self, args, optargs):
        return T('r.expr({', T(*[T(repr(k), ': ', v) for (k,v) in dict_items(optargs)], intsp=', '), '})')

class Var(RqlQuery):
    tt = pTerm.VAR

    def compose(self, args, optargs):
        return 'var_'+args[0]

class JavaScript(RqlTopLevelQuery):
    tt = pTerm.JAVASCRIPT
    st = "js"

class Http(RqlTopLevelQuery):
    tt = pTerm.HTTP
    st = "http"

class UserError(RqlTopLevelQuery):
    tt = pTerm.ERROR
    st = "error"

class Random(RqlTopLevelQuery):
    tt = pTerm.RANDOM
    st = "random"

class Changes(RqlMethodQuery):
    tt = pTerm.CHANGES
    st = "changes"

class Default(RqlMethodQuery):
    tt = pTerm.DEFAULT
    st = "default"

class ImplicitVar(RqlQuery):
    tt = pTerm.IMPLICIT_VAR

    def compose(self, args, optargs):
        return 'r.row'

class Eq(RqlBiCompareOperQuery):
    tt = pTerm.EQ
    st = "=="

class Ne(RqlBiCompareOperQuery):
    tt = pTerm.NE
    st = "!="

class Lt(RqlBiCompareOperQuery):
    tt = pTerm.LT
    st = "<"

class Le(RqlBiCompareOperQuery):
    tt = pTerm.LE
    st = "<="

class Gt(RqlBiCompareOperQuery):
    tt = pTerm.GT
    st = ">"

class Ge(RqlBiCompareOperQuery):
    tt = pTerm.GE
    st = ">="

class Not(RqlQuery):
    tt = pTerm.NOT

    def compose(self, args, optargs):
        if isinstance(self.args[0], Datum):
            args[0] = T('r.expr(', args[0], ')')
        return T('(~', args[0], ')')

class Add(RqlBiOperQuery):
    tt = pTerm.ADD
    st = "+"

class Sub(RqlBiOperQuery):
    tt = pTerm.SUB
    st = "-"

class Mul(RqlBiOperQuery):
    tt = pTerm.MUL
    st = "*"

class Div(RqlBiOperQuery):
    tt = pTerm.DIV
    st = "/"

class Mod(RqlBiOperQuery):
    tt = pTerm.MOD
    st = "%"

class Append(RqlMethodQuery):
    tt = pTerm.APPEND
    st = "append"

class Prepend(RqlMethodQuery):
    tt = pTerm.PREPEND
    st = "prepend"

class Difference(RqlMethodQuery):
    tt = pTerm.DIFFERENCE
    st = "difference"

class SetInsert(RqlMethodQuery):
    tt = pTerm.SET_INSERT
    st = "set_insert"

class SetUnion(RqlMethodQuery):
    tt = pTerm.SET_UNION
    st = "set_union"

class SetIntersection(RqlMethodQuery):
    tt = pTerm.SET_INTERSECTION
    st = "set_intersection"

class SetDifference(RqlMethodQuery):
    tt = pTerm.SET_DIFFERENCE
    st = "set_difference"

class Slice(RqlBracketQuery):
    tt = pTerm.SLICE
    st = 'slice'

    # Slice has a special bracket syntax, implemented here
    def compose(self, args, optargs):
        if self.bracket_operator:
            if needs_wrap(self.args[0]):
                args[0] = T('r.expr(', args[0], ')')
            return T(args[0], '[', args[1], ':', args[2], ']')
        else:
            return RqlBracketQuery.compose(self, args, optargs)

class Skip(RqlMethodQuery):
    tt = pTerm.SKIP
    st = 'skip'

class Limit(RqlMethodQuery):
    tt = pTerm.LIMIT
    st = 'limit'

class GetField(RqlBracketQuery):
    tt = pTerm.GET_FIELD
    st = 'get_field'

class Contains(RqlMethodQuery):
    tt = pTerm.CONTAINS
    st = 'contains'

class HasFields(RqlMethodQuery):
    tt = pTerm.HAS_FIELDS
    st = 'has_fields'

class WithFields(RqlMethodQuery):
    tt = pTerm.WITH_FIELDS
    st = 'with_fields'

class Keys(RqlMethodQuery):
    tt = pTerm.KEYS
    st = 'keys'

class Object(RqlMethodQuery):
    tt = pTerm.OBJECT
    st = 'object'

class Pluck(RqlMethodQuery):
    tt = pTerm.PLUCK
    st = 'pluck'

class Without(RqlMethodQuery):
    tt = pTerm.WITHOUT
    st = 'without'

class Merge(RqlMethodQuery):
    tt = pTerm.MERGE
    st = 'merge'

class Between(RqlMethodQuery):
    tt = pTerm.BETWEEN
    st = 'between'

class DB(RqlTopLevelQuery):
    tt = pTerm.DB
    st = 'db'

    def table_list(self, *args):
        return TableList(self, *args)

    def table_create(self, *args, **kwargs):
        kwargs.setdefault('primary_key', ())
        kwargs.setdefault('datacenter', ())
        kwargs.setdefault('durability', ())
        return TableCreate(self, *args, **kwargs)

    def table_drop(self, *args):
        return TableDrop(self, *args)

    def table(self, *args, **kwargs):
        kwargs.setdefault('use_outdated', ())
        return Table(self, *args, **kwargs)

class FunCall(RqlQuery):
    tt = pTerm.FUNCALL

    # This object should be constructed with arguments first, and the function itself as
    # the last parameter.  This makes it easier for the places where this object is
    # constructed.  The actual wire format is function first, arguments last, so we flip
    # them around before passing it down to the base class constructor.
    def __init__(self, *args):
        if len(args) == 0:
            raise RqlDriverError("Expected 1 or more argument(s) but found 0.")
        args = [func_wrap(args[-1])] + list(args[:-1])
        RqlQuery.__init__(self, *args)

    def compose(self, args, optargs):
        if len(args) != 2:
            return T('r.do(', T(T(*(args[1:]), intsp=', '), args[0], intsp=', '), ')')

        if isinstance(self.args[1], Datum):
            args[1] = T('r.expr(', args[1], ')')

        return T(args[1], '.do(', args[0], ')')

class Table(RqlQuery):
    tt = pTerm.TABLE
    st = 'table'

    def insert(self, *args, **kwargs):
        kwargs.setdefault('upsert', ())
        kwargs.setdefault('durability', ())
        kwargs.setdefault('return_vals', ())
        return Insert(self, *[expr(arg) for arg in args], **kwargs)

    def get(self, *args):
        return Get(self, *args)

    def get_all(self, *args, **kwargs):
        return GetAll(self, *args, **kwargs)

    def index_create(self, *args, **kwargs):
        kwargs.setdefault('multi', ())
        if len(args) > 1:
            args = [args[0]] + [func_wrap(arg) for arg in args[1:]]
        return IndexCreate(self, *args, **kwargs)

    def index_drop(self, *args):
        return IndexDrop(self, *args)

    def index_list(self, *args):
        return IndexList(self, *args)

    def index_status(self, *args):
        return IndexStatus(self, *args)

    def index_wait(self, *args):
        return IndexWait(self, *args)

    def sync(self, *args):
        return Sync(self, *args)

    def compose(self, args, optargs):
        if isinstance(self.args[0], DB):
            return T(args[0], '.table(', T(*(args[1:]), intsp=', '), ')')
        else:
            return T('r.table(', T(*(args), intsp=', '), ')')

class Get(RqlMethodQuery):
    tt = pTerm.GET
    st = 'get'

class GetAll(RqlMethodQuery):
    tt = pTerm.GET_ALL
    st = 'get_all'

class Reduce(RqlMethodQuery):
    tt = pTerm.REDUCE
    st = 'reduce'

class Sum(RqlMethodQuery):
    tt = pTerm.SUM
    st = 'sum'

class Avg(RqlMethodQuery):
    tt = pTerm.AVG
    st = 'avg'

class Min(RqlMethodQuery):
    tt = pTerm.MIN
    st = 'min'

class Max(RqlMethodQuery):
    tt = pTerm.MAX
    st = 'max'

class Map(RqlMethodQuery):
    tt = pTerm.MAP
    st = 'map'

class Filter(RqlMethodQuery):
    tt = pTerm.FILTER
    st = 'filter'

class ConcatMap(RqlMethodQuery):
    tt = pTerm.CONCATMAP
    st = 'concat_map'

class OrderBy(RqlMethodQuery):
    tt = pTerm.ORDERBY
    st = 'order_by'

class Distinct(RqlMethodQuery):
    tt = pTerm.DISTINCT
    st = 'distinct'

class Count(RqlMethodQuery):
    tt = pTerm.COUNT
    st = 'count'

class Union(RqlMethodQuery):
    tt = pTerm.UNION
    st = 'union'

class Nth(RqlBracketQuery):
    tt = pTerm.NTH
    st = 'nth'

class Match(RqlMethodQuery):
    tt = pTerm.MATCH
    st = 'match'

class Split(RqlMethodQuery):
    tt = pTerm.SPLIT
    st = 'split'

class Upcase(RqlMethodQuery):
    tt = pTerm.UPCASE
    st = 'upcase'

class Downcase(RqlMethodQuery):
    tt = pTerm.DOWNCASE
    st = 'downcase'

class IndexesOf(RqlMethodQuery):
    tt = pTerm.INDEXES_OF
    st = 'indexes_of'

class IsEmpty(RqlMethodQuery):
    tt = pTerm.IS_EMPTY
    st = 'is_empty'

class Group(RqlMethodQuery):
    tt = pTerm.GROUP
    st = 'group'

class InnerJoin(RqlMethodQuery):
    tt = pTerm.INNER_JOIN
    st = 'inner_join'

class OuterJoin(RqlMethodQuery):
    tt = pTerm.OUTER_JOIN
    st = 'outer_join'

class EqJoin(RqlMethodQuery):
    tt = pTerm.EQ_JOIN
    st = 'eq_join'

class Zip(RqlMethodQuery):
    tt = pTerm.ZIP
    st = 'zip'

class CoerceTo(RqlMethodQuery):
    tt = pTerm.COERCE_TO
    st = 'coerce_to'

class Ungroup(RqlMethodQuery):
    tt = pTerm.UNGROUP
    st = 'ungroup'

class TypeOf(RqlMethodQuery):
    tt = pTerm.TYPEOF
    st = 'type_of'

class Update(RqlMethodQuery):
    tt = pTerm.UPDATE
    st = 'update'

class Delete(RqlMethodQuery):
    tt = pTerm.DELETE
    st = 'delete'

class Replace(RqlMethodQuery):
    tt = pTerm.REPLACE
    st = 'replace'

class Insert(RqlMethodQuery):
    tt = pTerm.INSERT
    st = 'insert'

class DbCreate(RqlTopLevelQuery):
    tt = pTerm.DB_CREATE
    st = "db_create"

class DbDrop(RqlTopLevelQuery):
    tt = pTerm.DB_DROP
    st = "db_drop"

class DbList(RqlTopLevelQuery):
    tt = pTerm.DB_LIST
    st = "db_list"

class TableCreate(RqlMethodQuery):
    tt = pTerm.TABLE_CREATE
    st = "table_create"

class TableCreateTL(RqlTopLevelQuery):
    tt = pTerm.TABLE_CREATE
    st = "table_create"

class TableDrop(RqlMethodQuery):
    tt = pTerm.TABLE_DROP
    st = "table_drop"

class TableDropTL(RqlTopLevelQuery):
    tt = pTerm.TABLE_DROP
    st = "table_drop"

class TableList(RqlMethodQuery):
    tt = pTerm.TABLE_LIST
    st = "table_list"

class TableListTL(RqlTopLevelQuery):
    tt = pTerm.TABLE_LIST
    st = "table_list"

class IndexCreate(RqlMethodQuery):
    tt = pTerm.INDEX_CREATE
    st = 'index_create'

class IndexDrop(RqlMethodQuery):
    tt = pTerm.INDEX_DROP
    st = 'index_drop'

class IndexList(RqlMethodQuery):
    tt = pTerm.INDEX_LIST
    st = 'index_list'

class IndexStatus(RqlMethodQuery):
    tt = pTerm.INDEX_STATUS
    st = 'index_status'

class IndexWait(RqlMethodQuery):
    tt = pTerm.INDEX_WAIT
    st = 'index_wait'

class Sync(RqlMethodQuery):
    tt = pTerm.SYNC
    st = 'sync'

class Branch(RqlTopLevelQuery):
    tt = pTerm.BRANCH
    st = "branch"

class Any(RqlBoolOperQuery):
    tt = pTerm.ANY
    st = "or_"
    st_infix = "|"

class All(RqlBoolOperQuery):
    tt = pTerm.ALL
    st = "and_"
    st_infix = "&"

class ForEach(RqlMethodQuery):
    tt = pTerm.FOREACH
    st = 'for_each'

class Info(RqlMethodQuery):
    tt = pTerm.INFO
    st = 'info'

class InsertAt(RqlMethodQuery):
    tt = pTerm.INSERT_AT
    st = 'insert_at'

class SpliceAt(RqlMethodQuery):
    tt = pTerm.SPLICE_AT
    st = 'splice_at'

class DeleteAt(RqlMethodQuery):
    tt = pTerm.DELETE_AT
    st = 'delete_at'

class ChangeAt(RqlMethodQuery):
    tt = pTerm.CHANGE_AT
    st = 'change_at'

class Sample(RqlMethodQuery):
    tt = pTerm.SAMPLE
    st = 'sample'

class Json(RqlTopLevelQuery):
    tt = pTerm.JSON
    st = 'json'

class Args(RqlTopLevelQuery):
    tt = pTerm.ARGS
    st = 'args'

class ToISO8601(RqlMethodQuery):
    tt = pTerm.TO_ISO8601
    st = 'to_iso8601'

class During(RqlMethodQuery):
    tt = pTerm.DURING
    st = 'during'

class Date(RqlMethodQuery):
    tt = pTerm.DATE
    st = 'date'

class TimeOfDay(RqlMethodQuery):
    tt = pTerm.TIME_OF_DAY
    st = 'time_of_day'

class Timezone(RqlMethodQuery):
    tt = pTerm.TIMEZONE
    st = 'timezone'

class Year(RqlMethodQuery):
    tt = pTerm.YEAR
    st = 'year'

class Month(RqlMethodQuery):
    tt = pTerm.MONTH
    st = 'month'

class Day(RqlMethodQuery):
    tt = pTerm.DAY
    st = 'day'

class DayOfWeek(RqlMethodQuery):
    tt = pTerm.DAY_OF_WEEK
    st = 'day_of_week'

class DayOfYear(RqlMethodQuery):
    tt = pTerm.DAY_OF_YEAR
    st = 'day_of_year'

class Hours(RqlMethodQuery):
    tt = pTerm.HOURS
    st = 'hours'

class Minutes(RqlMethodQuery):
    tt = pTerm.MINUTES
    st = 'minutes'

class Seconds(RqlMethodQuery):
    tt = pTerm.SECONDS
    st = 'seconds'

class Time(RqlTopLevelQuery):
    tt = pTerm.TIME
    st = 'time'

class ISO8601(RqlTopLevelQuery):
    tt = pTerm.ISO8601
    st = 'iso8601'

class EpochTime(RqlTopLevelQuery):
    tt = pTerm.EPOCH_TIME
    st = 'epoch_time'

class Now(RqlTopLevelQuery):
    tt = pTerm.NOW
    st = 'now'

class InTimezone(RqlMethodQuery):
    tt = pTerm.IN_TIMEZONE
    st = 'in_timezone'

class ToEpochTime(RqlMethodQuery):
    tt = pTerm.TO_EPOCH_TIME
    st = 'to_epoch_time'

# Returns True if IMPLICIT_VAR is found in the subquery
def _ivar_scan(query):
    if not isinstance(query, RqlQuery):
        return False
    if isinstance(query, ImplicitVar):
        return True
    if any([_ivar_scan(arg) for arg in query.args]):
        return True
    if any([_ivar_scan(arg) for k,arg in dict_items(query.optargs)]):
        return True
    return False

# Called on arguments that should be functions
def func_wrap(val):
    val = expr(val)
    if _ivar_scan(val):
        return Func(lambda x: val)
    return val

class Func(RqlQuery):
    tt = pTerm.FUNC
    lock = Lock()
    nextVarId = 1

    def __init__(self, lmbd):
        vrs = []
        vrids = []
        for i in range_(lmbd.__code__.co_argcount):
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
    tt = pTerm.ASC
    st = 'asc'

class Desc(RqlTopLevelQuery):
    tt = pTerm.DESC
    st = 'desc'

class Literal(RqlTopLevelQuery):
    tt = pTerm.LITERAL
    st = 'literal'
