import query

import query_language_pb2 as p

###################
# PRETTY PRINTING #
###################

PRETTY_PRINT_EXPR_WRAPPED = "wrapped"
PRETTY_PRINT_EXPR_UNWRAPPED = "unwrapped"

class PrettyPrinter(object):
    def expr_wrapped(self, expr, backtrace_step):
        raise NotImplementedError()
    def expr_unwrapped(self, expr, backtrace_step):
        raise NotImplementedError()
    def mapping(self, mapping, backtrace_step):
        raise NotImplementedError()
    def reduction_base(self, expr, backtrace_step):
        raise NotImplementedError()
    def reduction_func(self, func, backtrace_step):
        raise NotImplementedError()

class ReprPrettyPrinter(object):
    def expr_wrapped(self, expr, backtrace_steps):
        assert isinstance(expr, query.ReadQuery)
        assert isinstance(backtrace_steps, list)
        string, wrapped = expr._inner.pretty_print(self)
        if wrapped == PRETTY_PRINT_EXPR_UNWRAPPED:
            string = "expr(%s)" % string
        return string

    def expr_unwrapped(self, expr, backtrace_steps):
        assert isinstance(expr, query.ReadQuery)
        assert isinstance(backtrace_steps, list)
        return expr._inner.pretty_print(self)[0]

PRETTY_PRINT_BEGIN_TARGET = "\0begin\0"
PRETTY_PRINT_END_TARGET = "\0end\0"

class BacktracePrettyPrinter(object):
    def __init__(self, current_backtrace, target_backtrace):
        self.current_backtrace = current_backtrace
        self.target_backtrace = target_backtrace

    def consider_backtrace(self, string, backtrace_steps):
        complete_backtrace = self.current_backtrace + backtrace_steps
        if complete_backtrace == self.target_backtrace:
            assert PRETTY_PRINT_BEGIN_TARGET not in string
            assert PRETTY_PRINT_END_TARGET not in string
            return PRETTY_PRINT_BEGIN_TARGET + string + PRETTY_PRINT_END_TARGET
        else:
            prefix_match_length = 0
            while True:
                if prefix_match_length == len(complete_backtrace):
                    # We're on the path to the target term.
                    return string
                elif prefix_match_length == len(self.target_backtrace):
                    # We're a sub-term of the target term.
                    if len(complete_backtrace) > len(self.target_backtrace) + 2 or len(string) > 60:
                        # Don't keep recursing for very long after finding the target
                        return "..." if len(string) > 8 else string
                    else:
                        return string
                else:
                    if complete_backtrace[prefix_match_length] == self.target_backtrace[prefix_match_length]:
                        prefix_match_length += 1
                    else:
                        # We're not on the path to the target term.
                        if len(complete_backtrace) > prefix_match_length + 2 or len(string) > 60:
                            # Don't keep recursing for very long on a side branch of the tree.
                            return "..." if len(string) > 8 else string
                        else:
                            return string

    def expr_wrapped(self, expr, backtrace_steps):
        string, wrapped = expr._inner.pretty_print(BacktracePrettyPrinter(self.current_backtrace + backtrace_steps, self.target_backtrace))
        if wrapped == PRETTY_PRINT_EXPR_UNWRAPPED:
            string = "expr(%s)" % string
        return self.consider_backtrace(string, backtrace_steps)

    def expr_unwrapped(self, expr, backtrace_step):
        string = expr._inner.pretty_print(BacktracePrettyPrinter(self.current_backtrace + backtrace_steps, self.target_backtrace))
        return self.consider_backtrace(string, backtrace_steps)

#####################################
# DATABASE AND TABLE ADMINISTRATION #
#####################################

class MetaQueryInner(object):
    def _write_table_op_query(self, parent):
        raise NotImplementedError()

class DBCreate(MetaQueryInner):
    def __init__(self, db_name, primary_datacenter):
        raise NotImplementedError()

class DBDrop(MetaQueryInner):
    def __init__(self, db_name):
        raise NotImplementedError()

class DBList(MetaQueryInner):
    def __init__(self):
        raise NotImplementedError()

class TableCreate(MetaQueryInner):
    def __init__(table_name, db_expr, primary_key='id'):
        raise NotImplementedError()

class TableDrop(MetaQueryInner):
    def __init__(table_name, db_expr):
        raise NotImplementedError()

class TableList(MetaQueryInner):
    def __init__(db_expr):
        raise NotImplementedError()

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
        self.entries = [query.expr(e) for e in entries]

    def _write_write_query(self, parent):
        parent.type = p.WriteQuery.INSERT
        self.table._write_ref_ast(parent.insert.table_ref)
        for entry in self.entries:
            entry._inner._write_ast(parent.insert.terms.add())

    def pretty_print(self, printer):
        return "%s.insert([%s])" % (
            printer.expr_wrapped(self.table, "table_ref"),
            ", ".join(printer.expr_unwrapped(e, ["terms:%d" % i]) for i, e in enumerate(self.entries)))

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
        return "%s.mutate(%s)" % (
            printer.expr_wrapped(self.parent_view, ["view"]),
            self.mapping._pretty_print(printer, ["mapping"]))

class InsertStream(WriteQueryInner):
    def __init__(self, table, stream):
        self.table = table
        self.stream = stream

    def _write_write_query(self, parent):
        parent.type = p.WriteQuery.INSERTSTREAM
        self.table._write_ref_ast(parent.insert_stream.table_ref)
        self.stream._inner._write_ast(parent.insert_stream.stream)

    def pretty_print(self, printer):
        return "%s.insert(%s)" % (
            printer.expr_wrapped(self.table, ["table_ref"]),
            printer.expr_unwrapped(self.stream, ["stream"]))

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
        self.value = dict((k, query.expr(v)) for k, v in value.iteritems())
    def _write_ast(self, parent):
        parent.type = p.Term.OBJECT
        for k, v in self.value:
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
    def _write_ast(self, parent):
        self._write_call(parent, self.builtin, *self.args)
    def pretty_print(self, printer):
        printed_args = []
        assert len(self.args) == len(self.arg_wrapped_flags)
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

class All(Builtin):
    builtin = p.Builtin.ALL
    format_string = "(%s & %s)"
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
        return ("%s.length()" % printer.expr_wrapped(self.parent, ["arg:0"]), PRETTY_PRINT_EXPR_WRAPPED)

class Attr(ExpressionInner):
    def __init__(self, parent, key):
        self.parent = query.expr(parent)
        self.key = key
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.GETATTR, self.parent)
        parent.call.builtin.attr = self.key
    def pretty_print(self, printer):
        return ("%s[%r]" % (printer.expr_wrapped(self.parent, ["arg:0"]), self.key), PRETTY_PRINT_EXPR_WRAPPED)

class ImplicitAttr(ExpressionInner):
    def __init__(self, attr):
        self.attr = attr
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.IMPLICIT_GETATTR)
        parent.call.builtin.attr = self.attr
    def pretty_print(self, printer):
        return ("R(%r)" % self.attr, PRETTY_PRINT_EXPR_WRAPPED)

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
        return ("%s[%s]" % (printer.expr_wrapped(self.stream, ["arg:0"]), printer.expr_unwrapped(self.index, ["arg:0"])), PRETTY_PRINT_EXPR_WRAPPED)

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
    def __init__(self, table, value):
        self.table = table
        self.key = "id"
        self.value = query.expr(value)

    def _write_ast(self, parent):
        parent.type = p.Term.GETBYKEY
        self.table._write_ref_ast(parent.get_by_key.table_ref)
        parent.get_by_key.attrname = self.key
        self.value._inner._write_ast(parent.get_by_key.key)

    def pretty_print(self, printer):
        return ("%s.get(%s)" % (
                printer.expr_wrapped(self.table, ["table_ref"]),
                printer.expr_unwrapped(self.value, ["key"])),
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
                self.reduce._pretty_print(printer, ["reduce", "body"])),
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
        return ("R(%r)" % ("$" + self.name), PRETTY_PRINT_EXPR_WRAPPED)

class Table(ExpressionInner):
    def __init__(self, table):
        assert isinstance(table, query.Table)
        self.table = table

    def _write_ast(self, parent):
        parent.type = p.Term.TABLE
        self.table._write_ref_ast(parent.table.table_ref)

    def pretty_print(self, printer):
        return ("table(%r)" % (self.table.db_expr.db_name + "." + self.table.table_name), PRETTY_PRINT_EXPR_WRAPPED)
