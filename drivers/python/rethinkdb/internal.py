import query
import query_language_pb2 as p

###################
# PRETTY PRINTING #
###################

# A note about pretty printing: The result of a pretty-print shouldn't contain
# newlines or tab characters. It may contain spaces.

PRETTY_PRINT_EXPR_WRAPPED = "wrapped"
PRETTY_PRINT_EXPR_UNWRAPPED = "unwrapped"

class PrettyPrinter(object):
    def expr_wrapped(self, expr, backtrace_steps):
        raise NotImplementedError()
    def expr_unwrapped(self, expr, backtrace_steps):
        raise NotImplementedError()
    def write_query(self, query, backtrace_steps):
        raise NotImplementedError()
    def simple_string(self, string, backtrace_steps):
        raise NotImplementedError()

class ReprPrettyPrinter(PrettyPrinter):
    # This implementation has a lot of assertions so that it validates the
    # implementations of `pretty_print()` on the various objects.

    def expr_wrapped(self, expr, backtrace_steps):
        assert isinstance(expr, query.ReadQuery)
        assert isinstance(backtrace_steps, list)
        string, wrapped = expr._inner.pretty_print(self)
        assert "\n" not in string
        if wrapped == PRETTY_PRINT_EXPR_UNWRAPPED:
            string = "expr(%s)" % string
        return string

    def expr_unwrapped(self, expr, backtrace_steps):
        assert isinstance(expr, query.ReadQuery)
        assert isinstance(backtrace_steps, list)
        string = expr._inner.pretty_print(self)[0]
        assert "\n" not in string
        return string

    def write_query(self, wq, backtrace_steps):
        assert isinstance(wq, query.WriteQuery)
        assert isinstance(backtrace_steps, list)
        string = wq._inner.pretty_print(self)
        assert "\n" not in string
        return string

    def meta_query(self, mq, backtrace_steps):
        assert isinstance(mq, query.MetaQuery)
        assert isinstance(backtrace_steps, list)
        string = mq._inner.pretty_print(self)
        assert "\n" not in string
        return string

    def simple_string(self, string, backtrace_steps):
        assert "\n" not in string
        return string

#####################################
# DATABASE AND TABLE ADMINISTRATION #
#####################################

class MetaQueryInner(object):
    def _write_meta_query(self, parent):
        raise NotImplementedError()
    def pretty_print(self, printer):
        raise NotImplementedError()

class DBCreate(MetaQueryInner):
    def __init__(self, db_name):
        assert isinstance(db_name, str)
        self.db_name = db_name
    def _write_meta_query(self, parent):
        parent.type = p.MetaQuery.CREATE_DB
        parent.db_name = self.db_name
    def pretty_print(self, printer):
        return "db_create(%r)" % self.db_name

class DBDrop(MetaQueryInner):
    def __init__(self, db_name):
        assert isinstance(db_name, str)
        self.db_name = db_name
    def _write_meta_query(self, parent):
        parent.type = p.MetaQuery.DROP_DB
        parent.db_name = self.db_name
    def pretty_print(self, printer):
        return "db_drop(%r)" % self.db_name

class DBList(MetaQueryInner):
    def _write_meta_query(self, parent):
        parent.type = p.MetaQuery.LIST_DBS
    def pretty_print(self, printer):
        return "db_list()"

class TableCreate(MetaQueryInner):
    def __init__(self, table_name, db_expr, primary_key, primary_datacenter, cache_size):
        assert isinstance(table_name, str)
        assert isinstance(db_expr, query.Database)
        assert (not primary_key) or isinstance(primary_key, str)
        assert (not primary_datacenter) or isinstance(primary_datacenter, str)
        assert (not cache_size) or isinstance(cache_size, int)
        self.table_name = table_name
        self.db_expr = db_expr
        self.primary_key = primary_key
        self.primary_datacenter = primary_datacenter
        self.cache_size = cache_size
    def _write_meta_query(self, parent):
        parent.type = p.MetaQuery.CREATE_TABLE
        parent.create_table.table_ref.db_name = self.db_expr.db_name
        parent.create_table.table_ref.table_name = self.table_name
        if self.primary_key:
            parent.create_table.primary_key = self.primary_key
        if self.primary_datacenter:
            parent.create_table.datacenter = self.primary_datacenter
        if self.cache_size:
            parent.create_table.cache_size = self.cache_size
    def pretty_print(self, printer):
        return "db(%s).table_create(%r, primary_datacenter=%s, primary_key=%r)" % (
            printer.simple_string(repr(self.db_expr.db_name), ["table_ref", "db_name"]),
            self.table_name,
            printer.simple_string(repr(self.primary_datacenter), ["datacenter"]),
            self.primary_key)

class TableDrop(MetaQueryInner):
    def __init__(self, table_name, db_expr):
        assert isinstance(table_name, str)
        assert isinstance(db_expr, query.Database)
        self.table_name = table_name
        self.db_expr = db_expr
    def _write_meta_query(self, parent):
        parent.type = p.MetaQuery.DROP_TABLE
        parent.drop_table.db_name = self.db_expr.db_name
        parent.drop_table.table_name = self.table_name
    def pretty_print(self, printer):
        return "db(%s).table_drop(%s)" % (
            printer.simple_string(repr(self.db_expr.db_name), ["db_name"]),
            printer.simple_string(repr(self.table_name), ["table_name"])
            )

class TableList(MetaQueryInner):
    def __init__(self, db_expr):
        assert isinstance(db_expr, query.Database)
        self.db_expr = db_expr
    def _write_meta_query(self, parent):
        parent.type = p.MetaQuery.LIST_TABLES
        parent.db_name = self.db_expr.db_name
    def pretty_print(self, printer):
        return "db(%s).table_list()" % (
            printer.simple_string(repr(self.db_expr.db_name), ["db_name"])
            )

#################
# WRITE QUERIES #
#################

class WriteQueryInner(object):
    def _write_write_query(self, parent):
        raise NotImplementedError()
    def pretty_print(self, printer):
        raise NotImplementedError()

class Insert(WriteQueryInner):
    def __init__(self, table, entries):
        self.table = table
        if isinstance(entries, query.StreamExpression):
            self.entries = [entries]
        else:
            self.entries = [query.expr(e) for e in entries]

    def _write_write_query(self, parent):
        parent.type = p.WriteQuery.INSERT
        self.table._write_ref_ast(parent.insert.table_ref)
        for entry in self.entries:
            entry._inner._write_ast(parent.insert.terms.add())

    def pretty_print(self, printer):
        return "%s.insert([%s])" % (
            printer.expr_wrapped(self.table, ["table_ref"]),
            ", ".join(printer.expr_unwrapped(e, ["term:%d" % i]) for i, e in enumerate(self.entries)))

class Delete(WriteQueryInner):
    def __init__(self, parent_view):
        self.parent_view = parent_view

    def _write_write_query(self, parent):
        parent.type = p.WriteQuery.DELETE
        self.parent_view._inner._write_ast(parent.delete.view)

    def pretty_print(self, printer):
        return "%s.delete()" % printer.expr_wrapped(self.parent_view, ["view"])

class Update(WriteQueryInner):
    def __init__(self, parent_view, mapping):
        self.parent_view = parent_view
        self.mapping = mapping

    def _write_write_query(self, parent):
        parent.type = p.WriteQuery.UPDATE
        self.parent_view._inner._write_ast(parent.update.view)
        self.mapping.write_mapping(parent.update.mapping)

    def pretty_print(self, printer):
        return "%s.update(%s)" % (
            printer.expr_wrapped(self.parent_view, ["view"]),
            self.mapping._pretty_print(printer, ["mapping"]))

class Mutate(WriteQueryInner):
    def __init__(self, parent_view, mapping):
        self.parent_view = parent_view
        self.mapping = mapping

    def _write_write_query(self, parent):
        parent.type = p.WriteQuery.MUTATE
        self.parent_view._inner._write_ast(parent.mutate.view)
        self.mapping.write_mapping(parent.mutate.mapping)

    def pretty_print(self, printer):
        return "%s.replace(%s)" % (
            printer.expr_wrapped(self.parent_view, ["view"]),
            self.mapping._pretty_print(printer, ["mapping"]))

class PointDelete(WriteQueryInner):
    def __init__(self, parent_view):
        self.parent_view = parent_view

    def _write_write_query(self, parent):
        parent.type = p.WriteQuery.POINTDELETE
        self.parent_view._inner._write_point_ast(parent.point_delete)

    def pretty_print(self, printer):
        return "%s.delete()" % printer.expr_wrapped(self.parent_view, ["view"])

class PointUpdate(WriteQueryInner):
    def __init__(self, parent_view, mapping):
        self.parent_view = parent_view
        self.mapping = mapping

    def _write_write_query(self, parent):
        parent.type = p.WriteQuery.POINTUPDATE
        self.mapping.write_mapping(parent.point_update.mapping)
        self.parent_view._inner._write_point_ast(parent.point_update)

    def pretty_print(self, printer):
        return "%s.update(%s)" % (
            printer.expr_wrapped(self.parent_view, ["view"]),
            self.mapping._pretty_print(printer, ["mapping"]))

class PointMutate(WriteQueryInner):
    def __init__(self, parent_view, mapping):
        self.parent_view = parent_view
        self.mapping = mapping

    def _write_write_query(self, parent):
        parent.type = p.WriteQuery.POINTMUTATE
        self.mapping.write_mapping(parent.point_mutate.mapping)
        self.parent_view._inner._write_point_ast(parent.point_mutate)

    def pretty_print(self, printer):
        return "%s.replace(%s)" % (
            printer.expr_wrapped(self.parent_view, ["view"]),
            self.mapping._pretty_print(printer, ["mapping"]))

################
# READ QUERIES #
################

class ExpressionInner(object):
    def _write_ast(self, parent):
        raise NotImplementedError()
    def _write_call(self, parent, builtin, *args):
        parent.type = p.Term.CALL
        parent.call.builtin.type = builtin
        for arg in args:
            arg._inner._write_ast(parent.call.args.add())
        return parent.call.builtin
    def pretty_print(self, pp):
        raise NotImplementedError()

class LiteralNull(ExpressionInner):
    def _write_ast(self, parent):
        parent.type = p.Term.JSON_NULL
    def pretty_print(self, printer):
        return ("None", PRETTY_PRINT_EXPR_UNWRAPPED)

class LiteralBool(ExpressionInner):
    def __init__(self, value):
        self.value = value
    def _write_ast(self, parent):
        parent.type = p.Term.BOOL
        parent.valuebool = self.value
    def pretty_print(self, printer):
        return (repr(self.value), PRETTY_PRINT_EXPR_UNWRAPPED)

class LiteralNumber(ExpressionInner):
    def __init__(self, value):
        self.value = value
    def _write_ast(self, parent):
        parent.type = p.Term.NUMBER
        parent.number = self.value
    def pretty_print(self, printer):
        return (repr(self.value), PRETTY_PRINT_EXPR_UNWRAPPED)

class LiteralString(ExpressionInner):
    def __init__(self, value):
        self.value = value
    def _write_ast(self, parent):
        parent.type = p.Term.STRING
        parent.valuestring = self.value
    def pretty_print(self, printer):
        return (repr(self.value), PRETTY_PRINT_EXPR_UNWRAPPED)

class LiteralArray(ExpressionInner):
    def __init__(self, value):
        self.value = [query.expr(e) for e in value]
    def _write_ast(self, parent):
        parent.type = p.Term.ARRAY
        for e in self.value:
            e._inner._write_ast(parent.array.add())
    def pretty_print(self, printer):
        return ("[" + ", ".join(printer.expr_unwrapped(e, ["elem:%d" % i]) for i, e in enumerate(self.value)) + "]", PRETTY_PRINT_EXPR_UNWRAPPED)

class LiteralObject(ExpressionInner):
    def __init__(self, value):
        for k, v in value.iteritems():
            assert isinstance(k, str)
        self.value = dict((k, query.expr(v)) for k, v in value.iteritems())
    def _write_ast(self, parent):
        parent.type = p.Term.OBJECT
        for k, v in self.value.iteritems():
            pair = parent.object.add()
            pair.var = k
            v._inner._write_ast(pair.term)
    def pretty_print(self, printer):
        return ("{" + ", ".join(repr(k) + ": " + printer.expr_unwrapped(v, ["key:%s" % k]) for k, v in self.value.iteritems()) + "}", PRETTY_PRINT_EXPR_UNWRAPPED)

class Javascript(ExpressionInner):
    def __init__(self, body):
        self.body = body

    def _write_ast(self, parent):
        parent.type = p.Term.JAVASCRIPT
        parent.javascript = self.body

    def pretty_print(self, printer):
        return ("js(body=%r)" % self.body, PRETTY_PRINT_EXPR_WRAPPED)

class ToArray(ExpressionInner):
    def __init__(self, stream):
        self.stream = stream
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.STREAMTOARRAY, self.stream)
    def pretty_print(self, printer):
        return ("%s.to_array()" % printer.expr_wrapped(self.stream, ["arg:0"]), PRETTY_PRINT_EXPR_WRAPPED)

class Builtin(ExpressionInner):
    # The subclass of `Builtin` is obligated to set the following attributes:
    # `builtin` - the protocol buffer enumeration value for the builtin
    # `format_string` - string with one `%s` per argument, for pretty-printing
    # `arg_wrapped_flags` - array of `PRETTY_PRINT_EXPR_WRAPPED` or
    #       `PRETTY_PRINT_EXPR_UNWRAPPED`, one per argument. Determines whether
    #       arguments will be pretty printed with `expr_wrapped()` or
    #       `expr_unwrapped()`.
    # `wrapped_flag` - either `PRETTY_PRINT_EXPR_WRAPPED` or
    #       `PRETTY_PRINT_EXPR_UNWRAPPED`, indicating whether the expression as
    #       a whole is wrapped or not.
    def __init__(self, *args):
        self.args = [query.expr(arg) for arg in args]
        assert len(self.args) == len(self.arg_wrapped_flags)
    def _write_ast(self, parent):
        self._write_call(parent, self.builtin, *self.args)
    def pretty_print(self, printer):
        printed_args = []
        assert len(self.args) == len(self.arg_wrapped_flags), "bad format for %r" % type(self)
        for i, (arg, wrapped) in enumerate(zip(self.args, self.arg_wrapped_flags)):
            if wrapped == PRETTY_PRINT_EXPR_WRAPPED:
                printed_args.append(printer.expr_wrapped(arg, ["arg:%d" % i]))
            elif wrapped == PRETTY_PRINT_EXPR_UNWRAPPED:
                printed_args.append(printer.expr_unwrapped(arg, ["arg:%d" % i]))
            else:
                raise ValueError("bad format for `arg_wrapped_flags`")
        return (self.format_string % tuple(printed_args), self.wrapped_flag)

class Add(Builtin):
    builtin = p.Builtin.ADD
    format_string = "(%s + %s)"
    arg_wrapped_flags = [PRETTY_PRINT_EXPR_WRAPPED, PRETTY_PRINT_EXPR_WRAPPED]
    wrapped_flag = PRETTY_PRINT_EXPR_WRAPPED

class Sub(Builtin):
    builtin = p.Builtin.SUBTRACT
    format_string = "(%s - %s)"
    arg_wrapped_flags = [PRETTY_PRINT_EXPR_WRAPPED, PRETTY_PRINT_EXPR_WRAPPED]
    wrapped_flag = PRETTY_PRINT_EXPR_WRAPPED

class Negate(Builtin):
    builtin = p.Builtin.SUBTRACT
    format_string = "-%s"
    arg_wrapped_flags = [PRETTY_PRINT_EXPR_WRAPPED]
    wrapped_flag = PRETTY_PRINT_EXPR_WRAPPED

class Mul(Builtin):
    builtin = p.Builtin.MULTIPLY
    format_string = "(%s * %s)"
    arg_wrapped_flags = [PRETTY_PRINT_EXPR_WRAPPED, PRETTY_PRINT_EXPR_WRAPPED]
    wrapped_flag = PRETTY_PRINT_EXPR_WRAPPED

class Div(Builtin):
    builtin = p.Builtin.DIVIDE
    format_string = "(%s / %s)"
    arg_wrapped_flags = [PRETTY_PRINT_EXPR_WRAPPED, PRETTY_PRINT_EXPR_WRAPPED]
    wrapped_flag = PRETTY_PRINT_EXPR_WRAPPED

class Mod(Builtin):
    builtin = p.Builtin.MODULO
    format_string = "(%s %% %s)"
    arg_wrapped_flags = [PRETTY_PRINT_EXPR_WRAPPED, PRETTY_PRINT_EXPR_WRAPPED]
    wrapped_flag = PRETTY_PRINT_EXPR_WRAPPED

class Any(Builtin):
    builtin = p.Builtin.ANY
    format_string = "(%s | %s)"
    arg_wrapped_flags = [PRETTY_PRINT_EXPR_WRAPPED, PRETTY_PRINT_EXPR_WRAPPED]
    wrapped_flag = PRETTY_PRINT_EXPR_WRAPPED

class Not(Builtin):
    builtin = p.Builtin.NOT
    format_string = "(~%s)"
    arg_wrapped_flags = [PRETTY_PRINT_EXPR_WRAPPED]
    wrapped_flag = PRETTY_PRINT_EXPR_WRAPPED

class Extend(Builtin):
    builtin = p.Builtin.MAPMERGE
    format_string = "%s.extend(%s)"
    arg_wrapped_flags = [PRETTY_PRINT_EXPR_WRAPPED, PRETTY_PRINT_EXPR_UNWRAPPED]
    wrapped_flag = PRETTY_PRINT_EXPR_WRAPPED

class Append(Builtin):
    builtin = p.Builtin.ARRAYAPPEND
    format_string = "%s.append(%s)"
    arg_wrapped_flags = [PRETTY_PRINT_EXPR_WRAPPED, PRETTY_PRINT_EXPR_UNWRAPPED]
    wrapped_flag = PRETTY_PRINT_EXPR_WRAPPED

class Comparison(Builtin):
    def _write_ast(self, parent):
        builtin = self._write_call(parent, p.Builtin.COMPARE, *self.args)
        builtin.comparison = self.comparison

class CompareLT(Comparison):
    comparison = p.Builtin.LT
    format_string = "(%s < %s)"
    arg_wrapped_flags = [PRETTY_PRINT_EXPR_WRAPPED, PRETTY_PRINT_EXPR_WRAPPED]
    wrapped_flag = PRETTY_PRINT_EXPR_WRAPPED

class CompareLE(Comparison):
    comparison = p.Builtin.LE
    format_string = "(%s <= %s)"
    arg_wrapped_flags = [PRETTY_PRINT_EXPR_WRAPPED, PRETTY_PRINT_EXPR_WRAPPED]
    wrapped_flag = PRETTY_PRINT_EXPR_WRAPPED

class CompareEQ(Comparison):
    comparison = p.Builtin.EQ
    format_string = "(%s == %s)"
    arg_wrapped_flags = [PRETTY_PRINT_EXPR_WRAPPED, PRETTY_PRINT_EXPR_WRAPPED]
    wrapped_flag = PRETTY_PRINT_EXPR_WRAPPED

class CompareNE(Comparison):
    comparison = p.Builtin.NE
    format_string = "(%s != %s)"
    arg_wrapped_flags = [PRETTY_PRINT_EXPR_WRAPPED, PRETTY_PRINT_EXPR_WRAPPED]
    wrapped_flag = PRETTY_PRINT_EXPR_WRAPPED

class CompareGT(Comparison):
    comparison = p.Builtin.GT
    format_string = "(%s > %s)"
    arg_wrapped_flags = [PRETTY_PRINT_EXPR_WRAPPED, PRETTY_PRINT_EXPR_WRAPPED]
    wrapped_flag = PRETTY_PRINT_EXPR_WRAPPED

class CompareGE(Comparison):
    comparison = p.Builtin.GE
    format_string = "(%s >= %s)"
    arg_wrapped_flags = [PRETTY_PRINT_EXPR_WRAPPED, PRETTY_PRINT_EXPR_WRAPPED]
    wrapped_flag = PRETTY_PRINT_EXPR_WRAPPED

# `All` is not a subclass of `Builtin` because it needs to work with an
# arbitrary number of arguments to support the syntactic sugar for `e.filter()`.
class All(ExpressionInner):
    def __init__(self, *args):
        self.args = [query.expr(a) for a in args]
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.ALL, *self.args)
    def pretty_print(self, printer):
        return ("(" + " & ".join(printer.expr_wrapped(a, ["arg:%d" % i]) for i, a in enumerate(self.args)) + ")",
            PRETTY_PRINT_EXPR_WRAPPED)

class Has(ExpressionInner):
    def __init__(self, parent, key):
        self.parent = query.expr(parent)
        self.key = key
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.HASATTR, self.parent)
        parent.call.builtin.attr = self.key
    def pretty_print(self, printer):
        return ("%s.has_attr(%r)" % (printer.expr_wrapped(self.parent, ["arg:0"]), self.key), PRETTY_PRINT_EXPR_WRAPPED)

class Length(ExpressionInner):
    def __init__(self, seq):
        self.seq = seq
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.LENGTH, self.seq)
    def pretty_print(self, printer):
        return ("%s.length()" % printer.expr_wrapped(self.seq, ["arg:0"]), PRETTY_PRINT_EXPR_WRAPPED)

class Attr(ExpressionInner):
    def __init__(self, parent, key):
        self.parent = query.expr(parent)
        self.key = key
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.GETATTR, self.parent)
        parent.call.builtin.attr = self.key
    def pretty_print(self, printer):
        return ("%s[%s]" % (
                printer.expr_wrapped(self.parent, ["arg:0"]),
                printer.simple_string(repr(self.key), ["attr"])),
            PRETTY_PRINT_EXPR_WRAPPED)

class ImplicitAttr(ExpressionInner):
    def __init__(self, attr):
        self.attr = attr
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.IMPLICIT_GETATTR)
        parent.call.builtin.attr = self.attr
    def pretty_print(self, printer):
        return ("r[%s]" % printer.simple_string(repr(self.attr), ["attr"]), PRETTY_PRINT_EXPR_WRAPPED)

class ImplicitVar(ExpressionInner):
    def _write_ast(self, parent):
        parent.type = p.Term.IMPLICIT_VAR
    def pretty_print(self, printer):
        return ("r['@']", PRETTY_PRINT_EXPR_WRAPPED)

class ToStream(ExpressionInner):
    def __init__(self, array):
        self.array = query.expr(array)
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.ARRAYTOSTREAM, self.array)
    def pretty_print(self, printer):
        return ("%s.to_stream()" % printer.expr_wrapped(self.array, ["arg:0"]), PRETTY_PRINT_EXPR_WRAPPED)

class Nth(ExpressionInner):
    def __init__(self, stream, index):
        self.stream = stream
        self.index = query.expr(index)
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.NTH, self.stream, self.index)
    def pretty_print(self, printer):
        return ("%s[%s]" % (printer.expr_wrapped(self.stream, ["arg:0"]), printer.expr_unwrapped(self.index, ["arg:1"])), PRETTY_PRINT_EXPR_WRAPPED)

class Slice(ExpressionInner):
    def __init__(self, parent, start, stop):
        self.parent = parent
        self.start = query.expr(start)
        self.stop = query.expr(stop)

    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.SLICE, self.parent, self.start, self.stop)

    def pretty_print(self, printer):
        return ("%s[%s:%s]" % (
                printer.expr_wrapped(self.parent, ["arg:0"]),
                printer.expr_unwrapped(self.start, ["arg:1"]),
                printer.expr_unwrapped(self.stop, ["arg:2"])),
            PRETTY_PRINT_EXPR_WRAPPED)

class Skip(ExpressionInner):
    def __init__(self, parent, offset):
        self.parent = parent
        self.offset = query.expr(offset)

    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.SKIP, self.parent, self.offset)

    def pretty_print(self, printer):
        return ("%s[%s:]" % (
                printer.expr_wrapped(self.parent, ["arg:0"]),
                printer.expr_unwrapped(self.offset, ["arg:1"])),
            PRETTY_PRINT_EXPR_WRAPPED)

class Filter(ExpressionInner):
    def __init__(self, parent, selector):
        self.parent = parent
        self.selector = selector

    def _write_ast(self, parent):
        builtin = self._write_call(parent, p.Builtin.FILTER, self.parent)
        self.selector.write_mapping(builtin.filter.predicate)

    def pretty_print(self, printer):
        return ("%s.filter(%s)" % (
                printer.expr_wrapped(self.parent, ["arg:0"]),
                self.selector._pretty_print(printer, ["predicate"])),
            PRETTY_PRINT_EXPR_WRAPPED)

class OrderBy(ExpressionInner):
    def __init__(self, parent, ordering):
        self.parent = parent
        self.ordering = ordering

    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.ORDERBY, self.parent)
        for key, val in self.ordering:
            elem = parent.call.builtin.order_by.add()
            elem.attr = key
            elem.ascending = bool(val)

    def pretty_print(self, printer):
        return ("%s.orderby(%s)" % (
                printer.expr_wrapped(self.parent, ["arg:0"]),
                ", ".join(repr(attr) for attr in self.ordering)),
            PRETTY_PRINT_EXPR_WRAPPED)

class Range(ExpressionInner):
    def __init__(self, parent, lowerbound, upperbound, attrname):
        self.parent = parent
        self.lowerbound = query.expr(lowerbound)
        self.upperbound = query.expr(upperbound)
        self.attrname = attrname

    def _write_ast(self, parent):
        builtin = self._write_call(parent, p.Builtin.RANGE, self.parent)
        builtin.range.attrname = self.attrname
        self.lowerbound._inner._write_ast(builtin.range.lowerbound)
        self.upperbound._inner._write_ast(builtin.range.upperbound)

    def pretty_print(self, printer):
        return ("%s.range(%s, %s%s)" % (
                printer.expr_wrapped(self.parent, ["arg:0"]),
                printer.expr_unwrapped(self.lowerbound, ["lowerbound"]),
                printer.expr_unwrapped(self.upperbound, ["upperbound"]),
                "" if self.attrname == "id" else ", attr_name = %r" % self.attrname),
            PRETTY_PRINT_EXPR_WRAPPED)

class Get(ExpressionInner):
    def __init__(self, table, key, attr_name):
        self.table = table
        self.key = query.expr(key)
        self.attr_name = attr_name

    def _write_ast(self, parent):
        parent.type = p.Term.GETBYKEY
        self._write_point_ast(parent.get_by_key)

    def _write_point_ast(self, parent):
        self.table._write_ref_ast(parent.table_ref)
        parent.attrname = self.attr_name
        self.key._inner._write_ast(parent.key)

    def pretty_print(self, printer):
        return ("%s.get(%s, attr_name = %r)" % (
                printer.expr_wrapped(self.table, ["table_ref"]),
                printer.expr_unwrapped(self.key, ["key"]),
                self.attr_name),
            PRETTY_PRINT_EXPR_WRAPPED)

class If(ExpressionInner):
    def __init__(self, test, true_branch, false_branch):
        # TODO: Actually support things other than `JSONExpression`
        self.test = query.expr(test)
        self.true_branch = query.expr(true_branch)
        self.false_branch = query.expr(false_branch)

    def _write_ast(self, parent):
        parent.type = p.Term.IF
        self.test._inner._write_ast(parent.if_.test)
        self.true_branch._inner._write_ast(parent.if_.true_branch)
        self.false_branch._inner._write_ast(parent.if_.false_branch)

    def pretty_print(self, printer):
        return ("if_then_else(%s, %s, %s)" % (
                printer.expr_unwrapped(self.test, ["test"]),
                printer.expr_unwrapped(self.true_branch, ["true_branch"]),
                printer.expr_unwrapped(self.false_branch, ["false_branch"])),
            PRETTY_PRINT_EXPR_WRAPPED)

class Map(ExpressionInner):
    def __init__(self, parent, mapping):
        self.parent = parent
        self.mapping = mapping

    def _write_ast(self, parent):
        builtin = self._write_call(parent, p.Builtin.MAP, self.parent)
        self.mapping.write_mapping(builtin.map.mapping)

    def pretty_print(self, printer):
        return ("%s.map(%s)" % (
                printer.expr_wrapped(self.parent, ["arg:0"]),
                self.mapping._pretty_print(printer, ["mapping"])),
            PRETTY_PRINT_EXPR_WRAPPED)

class ConcatMap(ExpressionInner):
    def __init__(self, parent, mapping):
        self.parent = parent
        self.mapping = mapping

    def _write_ast(self, parent):
        builtin = self._write_call(parent, p.Builtin.CONCATMAP, self.parent)
        self.mapping.write_mapping(builtin.concat_map.mapping)

    def pretty_print(self, printer):
        return ("%s.concat_map(%s)" % (
                printer.expr_wrapped(self.parent, ["arg:0"]),
                self.mapping._pretty_print(printer, ["mapping"])),
            PRETTY_PRINT_EXPR_WRAPPED)

class GroupedMapReduce(ExpressionInner):
    def __init__(self, input, group_mapping, value_mapping, reduction_base, reduction_func):
        self.input = input
        self.group_mapping = group_mapping
        self.value_mapping = value_mapping
        self.reduction_base = query.expr(reduction_base)
        self.reduction_func = reduction_func

    def _write_ast(self, parent):
        builtin = self._write_call(parent, p.Builtin.GROUPEDMAPREDUCE, self.input)
        self.group_mapping.write_mapping(builtin.grouped_map_reduce.group_mapping)
        self.value_mapping.write_mapping(builtin.grouped_map_reduce.value_mapping)
        self.reduction_func.write_reduction(builtin.grouped_map_reduce.reduction, self.reduction_base)

    def pretty_print(self, printer):
        return ("%s.grouped_map_reduce(%s, %s, %s, %s)" % (
                printer.expr_wrapped(self.input, ["arg:0"]),
                self.group_mapping._pretty_print(printer, ["group_mapping"]),
                self.value_mapping._pretty_print(printer, ["value_mapping"]),
                printer.expr_unwrapped(self.reduction_base, ["reduction", "base"]),
                self.reduction_func._pretty_print(printer, ["reduction", "body"])),
            PRETTY_PRINT_EXPR_WRAPPED)

class Distinct(ExpressionInner):
    def __init__(self, parent):
        self.parent = parent

    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.DISTINCT, self.parent)

    def pretty_print(self, printer):
        return ("%s.distinct()" % printer.expr_wrapped(self.parent, ["arg:0"]), PRETTY_PRINT_EXPR_WRAPPED)

class Reduce(ExpressionInner):
    def __init__(self, parent, base, reduction):
        self.parent = parent
        self.base = query.expr(base)
        self.reduction = reduction

    def _write_ast(self, parent):
        builtin = self._write_call(parent, p.Builtin.REDUCE, self.parent)
        self.reduction.write_reduction(builtin.reduce, self.base)

    def pretty_print(self, printer):
        return ("%s.reduce(%s, %s)" % (
                printer.expr_wrapped(self.parent, ["arg:0"]),
                printer.expr_unwrapped(self.base, ["reduce", "base"]),
                self.reduction._pretty_print(printer, ["reduce", "body"])),
            PRETTY_PRINT_EXPR_WRAPPED)

class Let(ExpressionInner):
    def __init__(self, expr, bindings):
        self.expr = expr
        self.bindings = []
        for var, val in bindings:
            self.bindings.append((var, query.expr(val)))

    def _write_ast(self, parent):
        parent.type = p.Term.LET
        for var, value in self.bindings:
            binding = parent.let.binds.add()
            binding.var = var
            value._inner._write_ast(binding.term)
        self.expr._inner._write_ast(parent.let.expr)

    def pretty_print(self, printer):
        return ("let(%s, %s)" % (
                ", ".join("(%r, %s)" % (var, printer.expr_unwrapped(val, ["bind:%s" % var]))
                    for var, val in self.bindings),
                printer.expr_unwrapped(self.expr, ["expr"])),
            PRETTY_PRINT_EXPR_WRAPPED)

class Var(ExpressionInner):
    def __init__(self, name):
        self.name = name

    def _write_ast(self, parent):
        parent.type = p.Term.VAR
        parent.var = self.name

    def pretty_print(self, printer):
        return ("%s" % self.name, PRETTY_PRINT_EXPR_WRAPPED)

class Table(ExpressionInner):
    def __init__(self, table):
        assert isinstance(table, query.Table)
        self.table = table

    def _write_ast(self, parent):
        parent.type = p.Term.TABLE
        self.table._write_ref_ast(parent.table.table_ref)

    def pretty_print(self, printer):
        res = ''
        if self.table.db_expr:
            res += "db(%r)." % self.table.db_expr.db_name
        res += "table(%r)" % self.table.table_name
        return (res, PRETTY_PRINT_EXPR_WRAPPED)
