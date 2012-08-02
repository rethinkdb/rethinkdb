import query

import query_language_pb2 as p

class Selector(object):
    def __new__(cls, parent, *args, **kwargs):
        """The resulting class should be a MultiRowSelection
        if the parent is, or a Stream if the parent is, or a
        JSONExpression if the parent is."""
        if isinstance(parent, query.MultiRowSelection):
            basetype = query.MultiRowSelection
        elif isinstance(parent, query.Stream):
            basetype = query.Stream
        elif isinstance(parent, query.JSONExpression):
            basetype = query.JSONExpression
        else:
            raise TypeError("cannot perform operation on incompatible type %r"
                            % cls.__name__)
        return type(cls.__name__, (cls, basetype), {})()

class Transformer(object):
    def __new__(cls, parent, *args, **kwargs):
        """The resulting class should be a Stream if the parent is,
        or a JSONExpression if the parent is."""
        if isinstance(parent, query.Stream):
            basetype = query.Stream
        elif isinstance(parent, query.JSONExpression):
            basetype = query.JSONExpression
        else:
            raise TypeError("cannot perform operation on incompatible type %r"
                            % cls.__name__)
        return type(cls.__name__, (cls, basetype), {})()

###########################
# DATABASE ADMINISTRATION #
###########################
class DBCreate(query.Expression):
    def __init__(db_name, primary_datacenter=None):
        pass

class DBDrop(query.Expression):
    def __init__(db_name):
        pass

class DBList(query.Expression):
    pass

########################
# TABLE ADMINISTRATION #
########################
class TableCreate(query.Expression):
    def __init__(table_name, db_expr=None, primary_key='id'):
        pass

class TableDrop(query.Expression):
    def __init__(table_name, db_expr=None):
        pass

class TableList(query.Expression):
    def __init__(db_expr=None):
        pass

##############
# OPERATIONS #
##############

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
