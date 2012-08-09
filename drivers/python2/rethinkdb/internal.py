import query

import query_language_pb2 as p

class PolymorphicOperation(object):
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

class WriteQuery(query.BaseExpression):
    def _finalize_query(self, root):
        root.type = p.Query.WRITE
        self._write_ast(root.write_query)

class Delete(WriteQuery):
    def __init__(self, parent_view):
        self.parent_view = parent_view

    def _write_ast(self, parent):
        parent.type = p.WriteQuery.DELETE
        self.parent_view._write_ast(parent.delete.view)

class Insert(WriteQuery):
    def __init__(self, table, entries):
        self.table = table
        self.entries = [query.expr(e) for e in entries]

    def _write_ast(self, parent):
        parent.type = p.WriteQuery.INSERT
        self.table._write_ref_ast(parent.insert.table_ref)
        for entry in self.entries:
            entry._write_ast(parent.insert.terms.add())

class InsertStream(WriteQuery):
    def __init__(self, table, stream):
        self.table = table
        self.stream = stream

    def _write_ast(self, parent):
        parent.type = p.WriteQuery.INSERTSTREAM
        self.table._write_ref_ast(parent.insert_stream.table_ref)
        self.stream._write_ast(parent.insert_stream.stream)

##############
# OPERATIONS #
##############

class Function(object):
    def __init__(self, body, *args):
        self.body = query.expr(body)
        self.args = args

    def write_mapping(self, mapping):
        if self.args:
            mapping.arg = self.args[0]
        else:
            mapping.arg = 'row'     # TODO: GET RID OF THIS
        self.body._write_ast(mapping.body)

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

class Element(JSONBuiltin):
    builtin = p.Builtin.ARRAYNTH

class ArrayNth(JSONBuiltin):
    builtin = p.Builtin.ARRAYNTH

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

class Nth(query.JSONExpression):
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

class Map(Transformer):
    def __init__(self, parent, mapping):
        self.parent = parent
        self.mapping = mapping

    def _write_ast(self, parent):
        builtin = self._write_call(parent, p.Builtin.MAP, self.parent)
        self.mapping.write_mapping(builtin.map.mapping)

class Distinct(Transformer):
    def __init__(self, parent):
        self.parent = parent

    def _write_ast(self, parent):
        self._write_call(parent, p.Builtin.DISTINCT, self.parent)

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
