from query import Expression, JSONExpression, expr

import query_language_pb2 as p

###########################
# DATABASE ADMINISTRATION #
###########################
class DBCreate(Expression):
    def __init__(db_name, primary_datacenter=None):
        pass

class DBDrop(Expression):
    def __init__(db_name):
        pass

class DBList(Expression):
    pass

########################
# TABLE ADMINISTRATION #
########################
class TableCreate(Expression):
    def __init__(table_name, db_expr=None, primary_key='id'):
        pass

class TableDrop(Expression):
    def __init__(table_name, db_expr=None):
        pass

class TableList(Expression):
    def __init__(db_expr=None):
        pass

##############
# OPERATIONS #
##############

class Comparison(JSONExpression):
    def __init__(self, *args):
        self.args = [expr(arg) for arg in args]
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

class Arithmetic(JSONExpression):
    def __init__(self, *args):
        self.args = [expr(arg) for arg in args]
    def _write_ast(self, parent):
        self._write_call(parent, self.builtin, *self.args)

class Add(Arithmetic):
    builtin = p.Builtin.ADD

class Sub(Arithmetic):
    builtin = p.Builtin.SUBTRACT

class Mul(Arithmetic):
    builtin = p.Builtin.MULTIPLY

class Div(Arithmetic):
    builtin = p.Builtin.DIVIDE

class Mod(Arithmetic):
    builtin = p.Builtin.MODULO

