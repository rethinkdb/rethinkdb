"""TODO"""

# __all__ = ['R', 'fn', 'expr', 'db', 'db_create', 'db_drop', 'db_list', 'table']

#############################
# VARIABLE/ATTRIBUTE ACCESS #
#############################
def R(name):
    """Create an expression that gets the value of a ReQL variable or
    JSON attribute. The argument `name` must be prefixed with `$` to
    access a variable, and `@` to access the attribute of JSON in an
    implicit scope.

    You an access inner attributes of JSON documents by separating
    names with a dot (`.`).

    See information on scoping rules for more details.

    :param name: The name of the variable (prefixed with `$`),
      implicit attribute (prefixed with `@`), or inner attributes
      (separated by `.`)
    :type name: str
    :returns: :class:`Expression`

    :Example:

    >>> q = table('users').insert({ 'name': Joe,
                                    'age': 30,
                                    'address': { 'city': 'Mountain View', 'state': 'CA' }
                                  })
    >>> q = table('users').filter(R('@age') == 30) # access attribute age from the implicit row scope
    >>> q = table('users').filter(R('@address.city') == 'Mountain View') # access subattribute city
                                                                         # of attribute address from
                                                                         # the implicit row scope
    >>> q = table('users').filter(bind('row', R('$row.age') == 30)) # access attribute age from the
                                                                    # variable 'row'
    >>> q = table('users').filter(bind('row', R('$row.address.city') == 'Mountain View')) # access subattribute city
                                                                                          # of attribute address from
                                                                                          # the variable 'row'
    >>> q = table('users').filter(bind('row', R('@age') == 30)) # error - binding a row disables implicit scope
    >>> q = table('users').filter(bind('row', R('$age') == 30)) # error - no variable 'age' is defined
    >>> q = table('users').filter(R('$age') == 30) # error - no variable 'age' is defined, use '@age' to access
                                                   # implicit scope
    """
    pass

def fn(param, *args):
    """Create a function with named parameters.
    See :func:`Selectable.filter` for examples.

    The last argument is the body of the function,
    and the other arguments are the parameter names.

    :param param: The name of the first parameter.
    :type param: str
    :param args: args[:-1] are names of additional parameters,
        args[-1] is the function body
    :type args: list(str/:class:`Expression`)

    >>> fn("x", R("$x") + 1)            # lambda x: x + 1
    >>> fn("x", "y", R("$x") + R("$y))  # lambda x, y: x + y
    """
    pass

#####################################
# SELECTORS - QUERYING THE DATABASE #
#####################################
class Selectable(object):
    """Defines operations that select data from a source (usually a
    table or an array). When applied to a :class:`Selection`, selector
    operations return another :class:`Selection`. When applied to a
    :class:`Stream`, they return another :class:`Stream`.

    Selection operations that work with JSON values are noted explicitly."""
    def between(start_key, end_key, start_inclusive=True, end_inclusive=True):
        """Select all elements between two keys.

        Also works on JSON arrays.

        :param start_key: the beginning of the range
        :type start_key: JSON value
        :param end_key: the end of the range
        :type end_key: JSON value
        :param start_inclusive: if True, includes rows with `start_key`
        :type start_inclusive: bool
        :param end_inclusive: if True, includes rows with `end_key`
        :type end_inclusive: bool
        :returns: :class:`Selection`, :class:`Stream`, :class:`Expression`

        >>> table('users').between(10, 20) # all users with ids between 10 and 20
        >>> expr([1, 2, 3, 4]).between(2, 4) # all elements >= 2 & <= 4
        [2, 3, 4]
        """
        pass

    def filter(selector):
        """Select all elements that fit the specified condition.

        Also works on JSON arrays.

        There are a number of ways to specify a selector for
        :func:`filter`. The simplest way is to pass a dict that
        defines a JSON document:

        >>> table('users').filter( { 'age': 30, 'state': 'CA'}) # select all thirty year olds in california

        We can also pass ReQL expressions directly. The above query is
        equivalent to the following query:

        >>> table('users').filter((R('@age') == 30) & (R('@state') == 'CA')))

        The values in a dict can contain ReQL expressions - they will
        get evaluated in order to evaluate the condition:

        >>> # Select all Californians whose age is equal to the number
        >>> # of colleges attended added to the number of jobs held
        >>> table('users').filter( { 'state': 'CA', 'age': R('@jobs_held') + R('@colleges_attended') })

        We can of course specify this query as a ReQL expression directly:

        >>> table('users').filter(R('@state') == 'CA' &
        >>>                       R('@age') == R('@jobs_held') + R('@colleges_attended'))

        We can use subqueries as well:

        >>> # Select all Californians whose age is equal to the number
        >>> of users in the database
        >>> table('users').filter( { 'state': 'CA', 'age': table('users').count() })

        So far we've been grabbing attributes from the implicit
        scope. We can bind the value of each row to a variable and
        operate on that:

        >>> table('users').filter(fn('row', R('$row.state') == 'CA' &
        >>>                                 R('$row.age') == R('$row.jobs_held') + R('$row.colleges_attended')))

        This type of syntax allows us to execute inner subqueries that
        refer to the outer row:

        >>> # Select all users whose age is equal to a number of blog
        >>> # posts written by all users with the same first name:
        >>> table('users').filter(fn('user',
        >>>     R('$user.age') == table('posts').filter(fn('post',
                  R('$post.author.first_name') == R('$user.first_name')))
                  .count()))

        :param selector: the constraint
        :type selector: dict, :class:`Expression`
        :returns: :class:`Selection`, :class:`Stream`, :class:`Expression`
        """
        pass

    def get(key):
        """Select a row by primary key. If the key doesn't exist, returns null.

        Not defined for JSON values.

        :param key: the key to look for
        :type start_key: JSON value
        :returns: :class:`RowSelection`, :class:`Expression`

        >>> q = table('users').get(10)  # get user with primary key 10
        """
        pass

    def nth(index):
        """Select the element at `index`.

        .. note:: ``e.nth(index)`` is equivalent to ``e[index]``.

        When applied to a :class:`Selection` returns a :class:`RowSelection`.
        When applied to a :class:`Stream` returns a JSON document (an :class:`Expression`).
        When applied to an array (:class:`Expression`) returns a JSON
        value (an :class:`Expression`).
        Not defined for, other JSON values, or :class:`RowSelection`.

        :param index: The element number to return.
        :type index: int
        :returns: :class:`RowSelection`, :class:`Expression`

        >>> expr([1, 2, 3, 4, 5]).nth(2)  # returns 3
        >>> table('users').nth(10) # returns 11th row
        >>> table('users')[10]     # returns 11th row
        """
        pass

    def skip(offset):
        """Skip elements before the element at `offset`.

        .. note:: ``e.skip(offset)`` is equivalent to ``e[offset:]``.

        :param offset: The number of elements to skip.
        :type offset: int
        :returns: :class:`Selection`, :class:`Expression`

        """
        pass

    def limit(count):
        """Select elements before the element at `count`.

        .. note:: ``e.limit(count)`` is equivalent to ``e[:count]``.
        """
        pass

    def orderby(*ordering):
        """Sort elements according to attributes specified by strings.

        Items are sorted in ascending order unless the attribute name starts
        with '-', which sorts the attribute in descending order.

        :param ordering: attribute names to order by
        :type ordering: list(str)

        >>> table('users').orderby('name')  # order users by name A-Z
        >>> table('users').orderby('-level', 'name') # levels high-low, then names A-Z
        """
        pass

    def random():
        """Select a random element."""
        pass

    def sample(count):
        """Select `count` elements at random."""
        pass


#####################
# TRANSFORMING DATA #
#####################
class Transformable(object):
    """Defines operations that transform data in various
    ways. Regardless of whether transfomers are applied to a a
    :class:`Selection`, or a :class:`Stream`, they always return a
    :class:`Stream`. Most transformation operators are defined on
    arrays, and some are defined on other types of JSON values."""

    def map(self, mapping):
        """Evaluate `mapping` for each element, with an implicit
        scope containing the element.

        :param mapping: The expression to evaluate
        :type mapping: :class:`Expression`

        >>> expr([1, 2, 3]).map(R('@') * 2) # gives [2, 4, 6]
        >>> table('users').map(R('age'))
        >>> table('users').map(fn('user', table('posts').filter({'userid': R('$user.id')})))"""

    def distinct(self, *attrs):


###################
# ReQL EXPRESSION #
###################
class Expression(Selectable, Transformable):
    """A base class for all ReQL expressions. An expression encodes an
    operation that can be evaluated on the server via
    :func:`rethinkdb.net.Connection.run` or
    :func:`self.run`. Expressions can be as simple as JSON values that
    get evaluated to themselves, or as complex as queries with
    multiple subqueries with table joins.

    Use :func:`expr` as a shorthand to create expressions for JSON values.

    :Example:

    >>> expr(1) # returns :class:`Expression` that encodes JSON value 1.
    >>> expr([1, 2]) # returns :class:`Expression` that encodes a JSON array.
    >>> expr("foo") # returns :class:`Expression` that encodes a JSON string.
    >>> expr({ 'name': 'Joe', 'age': 30 }) # # returns :class:`Expression` that encodes a JSON document.

    Expressions overload Python operators to enable comparisons on
    JSON values, as well as algebraic and logical operations.

    :Example:

    >>> conn.run(expr(1) < expr(2)) # Evaluates 1 < 2 on the server and returns True.
    >>> expr(1) < 2 # Whenever possible, ReQL converts Python types to expressions implicitly.
    >>> expr(1) + 2 # Addition. We can do `-`, `*`, `/`, and `%` in the same way.
    >>> expr(1) < 2 # We can also do `>`, `<=`, `>=`, `==`, etc.
    >>> expr(1) < 2 & 3 < 4 # We use `&` and `|` to encode `and` and `or` since we can't overload
    >>>                     # these in Python
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
        :returns: See the documentation for :func:`rethinkdb.net.Connection.run`

        :Example:

        >>> conn = rethinkdb.net.connect() # Connect to localhost, default port
        >>> q = table('db_name.table_name').insert({ 'a': 1, 'b': 2 }).run(conn)
        >>> q = table('db_name.table_name').all().run() # uses conn since it's the last created connection
        """
        pass

    def reduce(self, base, func):
        """Build up a result by repeatedly applying `func` to pairs of elements. Returns
        `base` if there are no elements.

        `base` should be an identity (`func(base, e) == e`).

        :Example:

        >>> expr([1, 2, 3]).reduce(0, fn('a', 'b', R('$a') + R('$b'))).run()
        6

        >>> table('users').reduce(0, fn('a', 'b', R('$a') + R('$b.credits)))"""


def expr(val):
    """Converts a python value to a ReQL :class:`Expression`.

    :param val: Any Python value that can be converted to JSON.
    :returns: :class:`Expression`

    :Example:

    >>> expr(1)
    >>> expr("foo")
    >>> expr(["foo", 1])
    >>> expr({ 'name': 'Joe', 'age': 30 })
    """
    return Expression(val)

##########################################
# DATA OBJECTS - SELECTION, STREAM, ETC. #
##########################################
class Selection(Expression):
    """A sequence of rows which can be read or written."""
    def delete(self):
        """Delete all rows in the selection from the database."""

    def update(self, func):
        """Update all rows in the selection by applying `func`.

        :Example:

        >>> table('users').filter(R('@bans') > 5).update(R('@').without('signature'))"""

class Stream(Expression):
    """A sequence of JSON values which can be read."""
    def to_array(self):
        """Convert the stream into a JSON array."""

class RowSelection(Selection, Expression):
    """A single row from a table which can be read or written."""
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

class Table(Selection):
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

def table(table_ref):
    """Get a reference to a table within a RethinkDB cluster.

    :param table_ref: Either a name of the table, or a name of the
      database followed by a period followed by a name of the table. If
      the database is omitted, the default database specified on
      the connection is used.
    :type table_ref: str
    :returns: :class:`Table` -- a reference to the specified table

    >>> q = table('table_name')         #
    >>> q = table('db_name.table_name') # equivalent to db('db_name').table('table_name')
    """
    pass


# this happens at the end since it's a circular import
import internal
