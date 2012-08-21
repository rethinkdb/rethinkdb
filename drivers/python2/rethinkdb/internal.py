import query

import query_language_pb2 as p

###########################
# DATABASE ADMINISTRATION #
###########################

class MetaQueryInner(object):
    def _write_table_op_query(self, parent):
        raise NotImplementedError()

class DBCreate(MetaQueryInner):
    def __init__(self, db_name, primary_datacenter=None):
        raise NotImplementedError()

class DBDrop(MetaQueryInner):
    def __init__(self, db_name):
        raise NotImplementedError

class DBList(MetaQueryInner):
    pass

########################
# TABLE ADMINISTRATION #
########################
class TableCreate(MetaQueryInner):
    def __init__(table_name, db_expr=None, primary_key='id'):
        raise NotImplementedError()

class TableDrop(MetaQueryInner):
    def __init__(table_name, db_expr=None):
        raise NotImplementedError()

class TableList(MetaQueryInner):
    def __init__(db_expr=None):
        raise NotImplementedError()

#################
# WRITE QUERIES #
#################

class WriteQueryInner(object):
    def _write_write_query(self, parent):
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

class Delete(WriteQueryInner):
    def __init__(self, parent_view):
        self.parent_view = parent_view

    def _write_write_query(self, parent):
        parent.type = p.WriteQuery.DELETE
        self.parent_view._inner._write_ast(parent.delete.view)

class Update(WriteQueryInner):
    def __init__(self, parent_view, mapping):
        self.parent_view = parent_view
        self.mapping = mapping

    def _write_write_query(self, parent):
        parent.type = p.WriteQuery.UPDATE
        self.parent_view._inner._write_ast(parent.update.view)
        self.mapping.write_mapping(parent.update.mapping)

class Mutate(WriteQueryInner):
    def __init__(self, parent_view, mapping):
        self.parent_view = parent_view
        self.mapping = mapping

    def _write_write_query(self, parent):
        parent.type = p.WriteQuery.MUTATE
        self.parent_view._inner._write_ast(parent.mutate.view)
        self.mapping.write_mapping(parent.mutate.mapping)

class InsertStream(WriteQueryInner):
    def __init__(self, table, stream):
        self.table = table
        self.stream = stream

    def _write_write_query(self, parent):
        parent.type = p.WriteQuery.INSERTSTREAM
        self.table._write_ref_ast(parent.insert_stream.table_ref)
        self.stream._inner._write_ast(parent.insert_stream.stream)

##############
# OPERATIONS #
##############

class ExpressionInner(object):
    def _write_ast(self, parent):
        raise NotImplementedError()
    def _write_call(self, parent, builtin, *args):
        parent.type = p.Term.CALL
        parent.call.builtin.type = builtin
        for arg in args:
            arg._inner._write_ast(parent.call.args.add())
        return parent.call.builtin

class JSONLiteral(ExpressionInner):
    """A literal JSON value."""

    def __init__(self, value):
        self.value = value
        if isinstance(value, bool):
            self.type = p.Term.BOOL
            self.valueattr = 'valuebool'
        elif isinstance(value, (int, float)):
            self.type = p.Term.NUMBER
            self.valueattr = 'number'
        elif isinstance(value, (str, unicode)):
            self.type = p.Term.STRING
            self.valueattr = 'valuestring'
        elif isinstance(value, dict):
            self.type = p.Term.OBJECT
        elif isinstance(value, list):
            self.type = p.Term.ARRAY
        elif value is None:
            self.type = p.Term.JSON_NULL
        else:
            raise TypeError("argument must be a JSON-compatible type (bool/int/float/str/unicode/dict/list),"
                            " not %r" % value.__class__.__name__)

    def _write_ast(self, parent):
        parent.type = self.type
        if self.type == p.Term.ARRAY:
            for value in self.value:
                query.expr(value)._inner._write_ast(parent.array.add())
        elif self.type == p.Term.OBJECT:
            for key, value in self.value.iteritems():
                pair = parent.object.add()
                pair.var = key
                query.expr(value)._inner._write_ast(pair.term)
        elif self.value is not None:
            setattr(parent, self.valueattr, self.value)

class Javascript(ExpressionInner):
    def __init__(self, body):
        self.body = body

    def _write_ast(self, parent):
        parent.type = p.Term.JAVASCRIPT
        parent.javascript = self.body

class ToArray(ExpressionInner):
    def __init__(self, stream):
        self.stream = stream
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.STREAMTOARRAY, self.stream)

class Count(ExpressionInner):
    def __init__(self, stream):
        self.stream = stream
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.LENGTH, self.stream)

class JSONBuiltin(ExpressionInner):
    def __init__(self, *args):
        self.args = [query.expr(arg) for arg in args]
    def _write_ast(self, parent):
        self._write_call(parent, self.builtin, *self.args)

class Add(JSONBuiltin):
    builtin = p.Builtin.ADD

class Sub(JSONBuiltin):
    builtin = p.Builtin.SUBTRACT

class Mul(JSONBuiltin):
    builtin = p.Builtin.MULTIPLY

class Div(JSONBuiltin):
    builtin = p.Builtin.DIVIDE

class Mod(JSONBuiltin):
    builtin = p.Builtin.MODULO

class Any(JSONBuiltin):
    builtin = p.Builtin.ANY

class All(JSONBuiltin):
    builtin = p.Builtin.ALL

class Not(JSONBuiltin):
    builtin = p.Builtin.NOT

class Extend(JSONBuiltin):
    builtin = p.Builtin.MAPMERGE

class Append(JSONBuiltin):
    builtin = p.Builtin.ARRAYAPPEND

class Comparison(JSONBuiltin):
    def _write_ast(self, parent):
        builtin = self._write_call(parent, p.Builtin.COMPARE, *self.args)
        builtin.comparison = self.comparison

class CompareLT(Comparison):
    comparison = p.Builtin.LT

class CompareLE(Comparison):
    comparison = p.Builtin.LE

class CompareEQ(Comparison):
    comparison = p.Builtin.EQ

class CompareNE(Comparison):
    comparison = p.Builtin.NE

class CompareGT(Comparison):
    comparison = p.Builtin.GT

class CompareGE(Comparison):
    comparison = p.Builtin.GE

class Has(ExpressionInner):
    def __init__(self, parent, key):
        self.parent = query.expr(parent)
        self.key = key
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.HASATTR, self.parent)
        parent.call.builtin.attr = self.key

class Length(ExpressionInner):
    def __init__(self, seq):
        self.seq = seq
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.LENGTH, self.seq)

class Attr(Has):
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.GETATTR, self.parent)
        parent.call.builtin.attr = self.key

class ImplicitAttr(ExpressionInner):
    def __init__(self, attr):
        self.attr = attr
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.IMPLICIT_GETATTR)
        parent.call.builtin.attr = self.attr

class StreamBuiltin(ExpressionInner):
    def __init__(self, stream, *args):
        self.stream = stream
        self.args = [query.expr(arg) for arg in args]
    def _write_ast(self, parent):
        self._write_call(parent, self.builtin, stream, *self.args)

class ToStream(ExpressionInner):
    def __init__(self, array):
        self.array = query.expr(array)
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.ARRAYTOSTREAM, self.array)

class Nth(ExpressionInner):
    def __init__(self, stream, index):
        self.stream = stream
        self.index = query.expr(index)
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.NTH, self.stream, self.index)

class Slice(ExpressionInner):
    def __init__(self, parent, start, stop):
        self.parent = parent
        self.start = query.expr(start)
        self.stop = query.expr(stop)

    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.SLICE, self.parent, self.start, self.stop)

class Skip(ExpressionInner):
    def __init__(self, parent, offset):
        self.parent = parent
        self.offset = query.expr(offset)

    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.SKIP, self.parent, self.offset)

class Filter(ExpressionInner):
    def __init__(self, parent, selector):
        self.parent = parent
        self.selector = selector

    def _write_ast(self, parent):
        builtin = self._write_call(parent, p.Builtin.FILTER, self.parent)
        self.selector.write_mapping(builtin.filter.predicate)

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

class Between(ExpressionInner):
    def __init__(self, parent, start_key, end_key, start_inclusive, end_inclusive):
        self.parent = parent
        self.start_key = query.expr(start_key)
        self.end_key = query.expr(end_key)
        self.start_inclusive = start_inclusive
        self.end_inclusive = end_inclusive

    def _write_ast(self, parent):
        builtin = self._write_call(parent, p.Builtin.RANGE, self.parent)
        builtin.range.attrname = 'id' # TODO: get rid of this ?
        self.start_key._inner._write_ast(builtin.range.lowerbound)
        self.end_key._inner._write_ast(builtin.range.upperbound)

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

class Map(ExpressionInner):
    def __init__(self, parent, mapping):
        self.parent = parent
        self.mapping = mapping

    def _write_ast(self, parent):
        builtin = self._write_call(parent, p.Builtin.MAP, self.parent)
        self.mapping.write_mapping(builtin.map.mapping)

class ConcatMap(ExpressionInner):
    def __init__(self, parent, mapping):
        self.parent = parent
        self.mapping = mapping

    def _write_ast(self, parent):
        builtin = self._write_call(parent, p.Builtin.CONCATMAP, self.parent)
        self.mapping.write_mapping(builtin.concat_map.mapping)

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
        self.reduction_base._inner._write_ast(builtin.grouped_map_reduce.reduction.base)
        builtin.grouped_map_reduce.reduction.var1 = self.reduction_func.args[0]
        builtin.grouped_map_reduce.reduction.var2 = self.reduction_func.args[1]
        self.reduction_func.body._inner._write_ast(builtin.grouped_map_reduce.reduction.body)

class Distinct(ExpressionInner):
    def __init__(self, parent):
        self.parent = parent

    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.DISTINCT, self.parent)

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

class Var(ExpressionInner):
    def __init__(self, name):
        self.name = name

    def _write_ast(self, parent):
        parent.type = p.Term.VAR
        parent.var = self.name

class Table(ExpressionInner):
    def __init__(self, table):
        assert isinstance(table, query.Table)
        self.table = table

    def _write_ast(self, parent):
        parent.type = p.Term.TABLE
        self.table._write_ref_ast(parent.table.table_ref)