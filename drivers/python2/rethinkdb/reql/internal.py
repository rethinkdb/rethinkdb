
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

