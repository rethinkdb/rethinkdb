import query

import query_language_pb2 as p

class PolymorphicOperation(object):
    """This class performs a bit of magic so that
    we can have polymorphic constructors -- when someone
    chains an operation off a Stream, they should get a subclass of Stream,
    and one off a JSONExpression should get a subclass of JSONExpression, etc.

    To do this, it inspects the type of the 1st argument, and constructs a new
    class with the appropriate base classes."""
    def __new__(cls, parent, *args, **kwargs):
        for base, output in cls.mapping:
            if isinstance(parent, base):
                return type(cls.__name__, (output,),
                            dict(cls.__dict__))(parent, *args, **kwargs)
        raise TypeError("cannot perform operation on incompatible type %r"
                        % parent.__class__.__name__)

class Selector(PolymorphicOperation):
    """The resulting class should be a MultiRowSelection
    if the parent is, or a Stream if the parent is, or a
    JSONExpression if the parent is."""
    mapping = [(query.MultiRowSelection, query.MultiRowSelection),
               (query.Stream, query.Stream),
               (query.JSONExpression, query.JSONExpression)]

class Transformer(PolymorphicOperation):
    """The resulting class should be a Stream if the parent is,
    or a JSONExpression if the parent is."""
    mapping = [(query.Stream, query.Stream),
               (query.JSONExpression, query.JSONExpression)]

class Picker(PolymorphicOperation):
    """The resulting class should be a RowSelection if the parent
    is a MultiRowSelection, or a JSONExpression if the parent is
    a Stream or JSONExpression."""
    mapping = [(query.MultiRowSelection, query.RowSelection),
               (query.Stream, query.JSONExpression),
               (query.JSONExpression, query.JSONExpression)]

###########################
# DATABASE ADMINISTRATION #
###########################
class DBCreate(query.BaseExpression):
    def __init__(db_name, primary_datacenter=None):
        pass

class DBDrop(query.BaseExpression):
    def __init__(db_name):
        pass

class DBList(query.BaseExpression):
    pass

########################
# TABLE ADMINISTRATION #
########################
class TableCreate(query.BaseExpression):
    def __init__(table_name, db_expr=None, primary_key='id'):
        pass

class TableDrop(query.BaseExpression):
    def __init__(table_name, db_expr=None):
        pass

class TableList(query.BaseExpression):
    def __init__(db_expr=None):
        pass

#################
# WRITE QUERIES #
#################

class Insert(query.WriteQuery):
    def __init__(self, table, entries):
        self.table = table
        self.entries = [query.expr(e) for e in entries]

    def _write_write_query(self, parent):
        parent.type = p.WriteQuery.INSERT
        self.table._write_ref_ast(parent.insert.table_ref)
        for entry in self.entries:
            entry._write_ast(parent.insert.terms.add())

class Delete(query.WriteQuery):
    def __init__(self, parent_view):
        self.parent_view = parent_view

    def _write_write_query(self, parent):
        parent.type = p.WriteQuery.DELETE
        self.parent_view._write_ast(parent.delete.view)

class Update(query.WriteQuery):
    def __init__(self, parent_view, mapping):
        self.parent_view = parent_view
        self.mapping = mapping

    def _write_write_query(self, parent):
        parent.type = p.WriteQuery.UPDATE
        self.parent_view._write_ast(parent.update.view)
        self.mapping.write_mapping(parent.update.mapping)

class Mutate(query.WriteQuery):
    def __init__(self, parent_view, mapping):
        self.parent_view = parent_view
        self.mapping = mapping

    def _write_write_query(self, parent):
        parent.type = p.WriteQuery.MUTATE
        self.parent_view._write_ast(parent.mutate.view)
        self.mapping.write_mapping(parent.mutate.mapping)

class InsertStream(query.WriteQuery):
    def __init__(self, table, stream):
        self.table = table
        self.stream = stream

    def _write_write_query(self, parent):
        parent.type = p.WriteQuery.INSERTSTREAM
        self.table._write_ref_ast(parent.insert_stream.table_ref)
        self.stream._write_ast(parent.insert_stream.stream)

##############
# OPERATIONS #
##############

class Javascript(query.JSONExpression):
    def __init__(self, body):
        self.body = body

    def _write_ast(self, parent):
        parent.type = p.Term.JAVASCRIPT
        parent.javascript = self.body

class ToArray(query.JSONExpression):
    def __init__(self, stream):
        self.stream = stream
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.STREAMTOARRAY, self.stream)

class Count(query.JSONExpression):
    def __init__(self, stream):
        self.stream = stream
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.LENGTH, self.stream)

class JSONBuiltin(query.JSONExpression):
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

class Has(query.JSONExpression):
    def __init__(self, parent, key):
        self.parent = query.expr(parent)
        self.key = key
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.HASATTR, self.parent)
        parent.call.builtin.attr = self.key

class Length(query.JSONExpression):
    def __init__(self, seq):
        self.seq = seq
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.LENGTH, self.seq)

class Attr(Has):
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.GETATTR, self.parent)
        parent.call.builtin.attr = self.key

class ImplicitAttr(query.JSONExpression):
    def __init__(self, attr):
        self.attr = attr
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.IMPLICIT_GETATTR)
        parent.call.builtin.attr = self.attr

class StreamBuiltin(query.Stream):
    def __init__(self, stream, *args):
        self.stream = stream
        self.args = [query.expr(arg) for arg in args]
    def _write_ast(self, parent):
        self._write_call(parent, self.builtin, stream, *self.args)

class ToStream(query.Stream):
    def __init__(self, array):
        self.array = query.expr(array)
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.ARRAYTOSTREAM, self.array)

class Nth(Picker):
    def __init__(self, stream, index):
        self.stream = stream
        self.index = query.expr(index)
    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.NTH, self.stream, self.index)

class Slice(Selector):
    def __init__(self, parent, start, stop):
        self.parent = parent
        self.start = query.expr(start)
        self.stop = query.expr(stop)

    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.SLICE, self.parent, self.start, self.stop)

class Skip(Selector):
    def __init__(self, parent, offset):
        self.parent = parent
        self.offset = query.expr(offset)

    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.SKIP, self.parent, self.offset)

class Filter(Selector):
    def __init__(self, parent, selector):
        self.parent = parent
        self.selector = selector

    def _write_ast(self, parent):
        builtin = self._write_call(parent, p.Builtin.FILTER, self.parent)
        self.selector.write_mapping(builtin.filter.predicate)

class OrderBy(Selector):
    def __init__(self, parent, ordering):
        self.parent = parent
        self.ordering = ordering

    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.ORDERBY, self.parent)
        for key, val in self.ordering:
            elem = parent.call.builtin.order_by.add()
            elem.attr = key
            elem.ascending = bool(val)

class Between(Selector):
    def __init__(self, parent, start_key, end_key, start_inclusive, end_inclusive):
        self.parent = parent
        self.start_key = query.expr(start_key)
        self.end_key = query.expr(end_key)
        self.start_inclusive = start_inclusive
        self.end_inclusive = end_inclusive

    def _write_ast(self, parent):
        builtin = self._write_call(parent, p.Builtin.RANGE, self.parent)
        builtin.range.attrname = 'id' # TODO: get rid of this ?
        self.start_key._write_ast(builtin.range.lowerbound)
        self.end_key._write_ast(builtin.range.upperbound)

class Get(query.RowSelection):
    def __init__(self, table, value):
        self.table = table
        self.key = "id"
        self.value = query.expr(value)

    def _write_ast(self, parent):
        parent.type = p.Term.GETBYKEY
        self.table._write_ref_ast(parent.get_by_key.table_ref)
        parent.get_by_key.attrname = self.key
        self.value._write_ast(parent.get_by_key.key)

class If(query.JSONExpression):
    def __init__(self, test, true_branch, false_branch):
        # TODO: Actually support things other than `JSONExpression`
        self.test = query.expr(test)
        self.true_branch = query.expr(true_branch)
        self.false_branch = query.expr(false_branch)

    def _write_ast(self, parent):
        parent.type = p.Term.IF
        self.test._write_ast(parent.if_.test)
        self.true_branch._write_ast(parent.if_.true_branch)
        self.false_branch._write_ast(parent.if_.false_branch)

class Map(Transformer):
    def __init__(self, parent, mapping):
        self.parent = parent
        self.mapping = mapping

    def _write_ast(self, parent):
        builtin = self._write_call(parent, p.Builtin.MAP, self.parent)
        self.mapping.write_mapping(builtin.map.mapping)

class ConcatMap(Transformer):
    def __init__(self, parent, mapping):
        self.parent = parent
        self.mapping = mapping

    def _write_ast(self, parent):
        builtin = self._write_call(parent, p.Builtin.CONCATMAP, self.parent)
        self.mapping.write_mapping(builtin.concat_map.mapping)

class GroupedMapReduce(query.JSONExpression):
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
        self.reduction_base._write_ast(builtin.grouped_map_reduce.reduction.base)
        builtin.grouped_map_reduce.reduction.var1 = self.reduction_func.args[0]
        builtin.grouped_map_reduce.reduction.var2 = self.reduction_func.args[1]
        self.reduction_func.body._write_ast(builtin.grouped_map_reduce.reduction.body)

class Distinct(Transformer):
    def __init__(self, parent):
        self.parent = parent

    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.DISTINCT, self.parent)

class Reduce(Transformer):
    def __init__(self, parent, base, reduction):
        self.parent = parent
        self.base = query.expr(base)
        self.reduction = reduction

    def _write_ast(self, parent):
        builtin = self._write_call(parent, p.Builtin.REDUCE, self.parent)
        self.base._write_ast(builtin.reduce.base)
        self.reduction.write_reduction(builtin.reduce, self.base)

class Let(Selector):
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
            value._write_ast(binding.term)
        self.expr._write_ast(parent.let.expr)

class Var(query.JSONExpression):
    def __init__(self, name):
        self.name = name

    def _write_ast(self, parent):
        parent.type = p.Term.VAR
        parent.var = self.name
