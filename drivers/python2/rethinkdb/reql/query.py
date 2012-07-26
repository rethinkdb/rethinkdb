

###################
# ReQL EXPRESSION #
###################
class Expression():
    """A base class for all ReQL expressions. An expression encodes an
    operation that can be evaluated on the server via
    :func:`rethinkdb.net.Connection.run` or
    :func:`self.run`. Expressions can be as simple as JSON values that
    get evaluated to themselves, or as complex as queries with
    multiple subqueries with table joins.
    """
    def run(conn=None):
        """Evaluate the expression on the server using the connection
        specified by `conn`. If `conn` is empty, uses the last created
        connection (located in :data:`rethinkdb.net.last_connection`).

        This method is shorthand for
        :func:`rethinkdb.net.Connection.run` - see its documentation
        for more details.
        
        :param conn: An optional connection object used to evaluate
          the expression on the RethinkDB server.
        :type conn: :class:`rethinkdb.net.Connection`
      
        :Example:
        
        >>> conn = rethinkdb.net.connect() # Connect to localhost, default port
        >>> q = table('db_name.table_name').insert({ 'a': 1, 'b': 2 }).run() # uses conn since it's the last created connection
        >>> q = table('db_name.table_name').all().run() # uses conn since it's the last created connection
        """
        pass

#####################################
# SELECTORS - QUERYING THE DATABASE #
#####################################
class SelectorMixin():
    """Foo"""
    def filter(selector):
        """Foo"""
        pass

    def range(start_key, end_key, start_inclusive=True, end_inclusive=True):
        """Foo"""
        pass

    def get(key):
        """Foo"""
        pass

    def nth(index):
        """Foo"""
        pass

    def skip(count):
        """Foo"""
        pass

    def limit(count):
        """Foo"""
        pass

    def random():
        """Foo"""
        pass

    def sample(count):
        """Foo"""
        pass

##########################################
# DATA OBJECTS - SELECTION, STREAM, ETC. #
##########################################
class Selection(Expression, SelectorMixin):
    """Foo"""
    pass

class RowSelection(Expression):
    """Foo"""
    pass

class Stream(Expression, SelectorMixin):
    """Foo"""
    pass

###########################
# DATABASE ADMINISTRATION #
###########################
def db_create(db_name, primary_datacenter=None):
    """Create a ReQL expression that creates a database within a
    RethinkDB cluster. A RethinkDB database is an object that contains
    related tables as well as configuration options that apply to
    these tables.
    
    When run via :func:`rethinkdb.net.Connection.run` or
    :func:`Expression.run`, `run` has no return value in case of
    success, and raises :class:`rethinkdb.net.QueryError` in case of
    failure.
    
    :param db_name: The name of the database to be created.
    :type db_name: str
    :param primary_datacenter: An optional name of the primary
      datacenter to be used for this database. If this argument is
      omitted, the cluster-level default datacenter will be used as
      primary for this database.
    :type primary_datacenter: str
    :returns: :class:`Expression` -- a ReQL expression that encodes
      the database creation operation.
      
    :Example:
    
    >>> q = db_create('db_name')
    >>> q = db_create('db_name', primary_datacenter='us_west')
    """
    pass

def db_drop(db_name):
    """Create a ReQL expression that drops a database within a
    RethinkDB cluster.
    
    When run via :func:`rethinkdb.net.Connection.run` or
    :func:`Expression.run`, `run` has no return value in case of
    success, and raises :class:`rethinkdb.net.QueryError` in case of
    failure.
    
    :param db_name: The name of the database to be dropped.
    :type db_name: str
    :returns: :class:`Expression` -- a ReQL expression that encodes
      the database dropping operation.
      
    :Example:
    
    >>> q = db_drop('db_name')
    """
    pass

def db_list():
    """Create a ReQL expression that lists all databases within a
    RethinkDB cluster.
    
    When run via :func:`rethinkdb.net.Connection.run` or
    :func:`Expression.run`, `run` returns a list of database name
    strings in case of success, and raises
    :class:`rethinkdb.net.QueryError` in case of failure.
    
    :returns: :class:`Expression` -- a ReQL expression that encodes
      the database listing operation.
      
    :Example:
    
    >>> q = db_list() # returns a list of names, e.g. ['db1', 'db2', 'db3']
    """
    pass

##############################################
# LEAF SELECTORS - DATABASE AND TABLE ACCESS #
##############################################
class Database():
    """A ReQL expression that encodes a RethinkDB database. Most
    database-related operations (including table access) can be
    chained off of this object."""
    def __init__(db_name):
        """Use :func:`rethinkdb.query.db` to create this object.
        
        :param db_name: Name of the databases to access.
        :type db_expr: str
        """
        pass

    def create(table_name, primary_key=None):
        """Create a ReQL expression that creates a table within this
        RethinkDB database. A RethinkDB table is an object that
        contains JSON documents.
    
        When run via :func:`rethinkdb.net.Connection.run` or
        :func:`Expression.run`, `run` has no return value in case of
        success, and raises :class:`rethinkdb.net.QueryError` in case
        of failure.
    
        :param table_name: The name of the table to be created.
        :type table_name: str
        :param primary_key: An optional name of the JSON attribute
          that will be used as a primary key for the document. If
          missing, defaults to 'id'.
        :type primary_key: str
        :returns: :class:`Expression` -- a ReQL expression that
          encodes the table creation operation.
      
        :Example:
        
        >>> q = db('db_name').create('posts') # uses primary key 'id'
        >>> q = db('db_name').create('users', primary_key='user_id')
        """
        pass

    def drop(table_name):
        """Create a ReQL expression that drops a table within this
        RethinkDB database. 
    
        When run via :func:`rethinkdb.net.Connection.run` or
        :func:`Expression.run`, `run` has no return value in case of
        success, and raises :class:`rethinkdb.net.QueryError` in case
        of failure.
    
        :param table_name: The name of the table to be dropped.
        :type table_name: str
        :returns: :class:`Expression` -- a ReQL expression that
          encodes the table creation operation.
      
        :Example:
        
        >>> q = db('db_name').drop('posts')
        """
        pass
    
    def list():
        """Create a ReQL expression that lists all tables within this
        RethinkDB database.
    
        When run via :func:`rethinkdb.net.Connection.run` or
        :func:`Expression.run`, `run` returns a list of table name
        strings in case of success, and raises
        :class:`rethinkdb.net.QueryError` in case of failure.
    
        :returns: :class:`TableCreate` -- a ReQL expression that
          encodes the table creation operation.
      
        :Example:
        
        >>> q = db('db_name').list() # returns a list of tables, e.g. ['table1', 'table2']
        """
        pass
    
    def table(table_name):
        """Create a ReQL expression that encodes a table within this
        RethinkDB database. This function is a shortcut for
        constructing the :class:`Table` object.

        Use :func:`rethinkdb.query.table` as a shortcut for this
        method.
    
        :returns: :class:`Table` -- a ReQL expression that encodes the
          table expression.
        """
        pass

def db(db_name):
    """Create a ReQL expression that encodes a database within a
    RethinkDB cluster. This function is a shortcut for constructing
    the :class:`Database` object.
    
    :returns: :class:`Database` -- a ReQL expression that encodes the
      database expression.
      
    :Example:
        
    >>> q = db('db_name')
    """
    pass

class Table(SelectorMixin):
    """A ReQL expression that encodes a RethinkDB table. Most data
    manipulation operations (such as inserting, selecting, and
    updating data) can be chained off of this object."""

    def __init__(table_name, db_expr=None):
        """Use :func:`rethinkdb.query.table` as a shortcut to create
        this object.
        
        :param table_name: Name of the databases to access.
        :type table_name: str
        :param db_expr: An optional database where this table
          resides. If missing, use default database specified on the
          connection object.
        :type db_expr: :class:`Database`
        """
        pass

    def all():
        """Foo"""
        pass

def table(table_ref):
    """Create a ReQL expression that encodes a table within a
    RethinkDB cluster. This function is a shortcut for constructing
    the :class:`Table` object.

    :param table_ref: Either a name of the table, or a name of the
      database followed by a dot followed by a name of the table. If
      the database is omitted, uses the default database specified on
      the connection.
    :type table_ref: str
    :returns: :class:`Table` -- a ReQL expression that encodes the
      table expression.

    :Example:
    
    >>> q = table('table_name')
    >>> q = table('db_name.table_name') # equivalent to db('db_name').table('table_name')
    """
    pass

