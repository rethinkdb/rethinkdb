"""TODO"""

# __all__ = ['R', 'fn', 'expr', 'db', 'db_create', 'db_drop', 'db_list', 'table']

import types

import query_language_pb2 as p

#############################
# VARIABLE/ATTRIBUTE ACCESS #
#############################
def R(name):
    """Get the value of a variable or attribute.

    To get a variable, prefix the name with `$`.

    >>> R('$user')

    To get attributes of variables, use dot notation.

    >>> R('$user.name')
    >>> R('$user.options.ads')

    Filter and map bind the current element to the implicit variable.

    To access an attribute of the implicit variable, pass the attribute name.

    >>> R('name')
    >>> R('options.ads')

    To get the implicit variable, use '@'.

    >>> R('@')

    For attributes that would be misinterpreted, use alternative notations.

    >>> R('@.$special')     # get implicit var's "$special" attribute
    >>> R('@')['$special']  # the same
    >>> R('@')['a.b.c']     # get an attribute named "a.b.c"

    See information on scoping rules for more details.

    :param name: The name of the variable (prefixed with `$`),
      implicit attribute (prefixed with `@`), or inner attributes
      (separated by `.`)
    :type name: str
    :returns: :class:`JSONExpression`

    >>> table('users').insert({ 'name': Joe,
                                'age': 30,
                                'address': { 'city': 'Mountain View', 'state': 'CA' }
                              }).run()
    >>> table('users').filter(R('age') == 30) # access attribute age from the implicit row variable
    >>> table('users').filter(R('address.city') == 'Mountain View') # access subattribute city
                                                                    # of attribute address from
                                                                    # the implicit row variable
    >>> table('users').filter(fn('row', R('$row.age') == 30)) # access attribute age from the
                                                              # variable 'row'
    >>> table('users').filter(fn('row', R('$row.address.city') == 'Mountain View')) # access subattribute city
                                                                                     # of attribute address from
                                                                                     # the variable 'row'
    >>> table('users').filter(fn('row', R('age') == 30)) # error - binding a row disables implicit scope
    >>> table('users').filter(fn('row', R('$age') == 30)) # error - no variable 'age' is defined
    >>> table('users').filter(R('$age') == 30) # error - no variable '$age' is defined, use 'age'
    """
    if name.startswith('$'):
        if '.' not in name:
            return internal.Var(name[1:])
        raise NotImplementedError("$ with . not handled")
    if name.startswith('@'):
        raise NotImplementedError("@ not handled")
    if '.' in name:
        raise NotImplementedError(". not handled")
    return internal.ImplicitAttr(name)

def fn(*x):
    """Create a function.
    See :func:`Selectable.filter` for examples.

    The last argument is the body of the function,
    and the other arguments are the parameter names.

    :param args: names of parameters
    :param body: body of function
    :type body: :class:`JSONExpression` or :class:`StreamExpression`
    :rtype: :class:`rethinkdb.internal.JSONFunction` or :class:`rethinkdb.internal.StreamFunction`

    >>> fn(3)                           # lambda: 3
    >>> fn("x", R("$x") + 1)            # lambda x: x + 1
    >>> fn("x", "y", R("$x") + R("$y))  # lambda x, y: x + y
    """
    body = x[-1]
    args = x[:-1]
    if isinstance(body, Stream):
        return internal.StreamFunction(body, *args)
    else:
        return internal.JSONFunction(body, *args)

def js(expr=None, body=None):
    if (expr is not None) + (body is not None) != 1:
        raise ValueError('exactly one of expr or body must be passed')
    if body is not None:
        return internal.Javascript(body)
    else:
        return internal.Javascript(u'return (%s);' % expr)

def let(*bindings):
    if len(bindings) < 2:
        raise ValueError("need at least one binding")
    return internal.Let(bindings[-1], bindings[:-1])

#####################################
# SELECTORS - QUERYING THE DATABASE #
#####################################
class BaseExpression(object):
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
        :returns: See the documentation for :func:`rethinkdb.net.Connection.run`

        >>> conn = rethinkdb.net.connect() # Connect to localhost, default port
        >>> res = table('db_name.table_name').insert({ 'a': 1, 'b': 2 }).run(conn)
        >>> res = table('db_name.table_name').all().run() # uses conn since it's the last created connection
        """
        raise NotImplementedError

class Expression(BaseExpression):
    """Base class for everything"""

    def _finalize_query(self, root):
        root.type = p.Query.READ
        self._write_ast(root.read_query.term)

    def _write_call(self, parent, builtin, *args):
        parent.type = p.Term.CALL
        parent.call.builtin.type = builtin
        for arg in args:
            arg._write_ast(parent.call.args.add())
        return parent.call.builtin

    def between(self, start_key, end_key, start_inclusive=True, end_inclusive=True):
        """Select all elements between two keys.

        This is a Selector.

        :param start_key: the beginning of the range
        :type start_key: JSON value
        :param end_key: the end of the range
        :type end_key: JSON value
        :param start_inclusive: if True, includes rows with `start_key`
        :type start_inclusive: bool
        :param end_inclusive: if True, includes rows with `end_key`
        :type end_inclusive: bool
        :returns: :class:`Stream`, :class:`MultiRowSelection`, :class:`JSONExpression` (depends on input)

        >>> table('users').between(10, 20) # all users with ids between 10 and 20
        >>> expr([1, 2, 3, 4]).between(2, 4) # [2, 3, 4]
        """
        return internal.Between(self, start_key, end_key, start_inclusive, end_inclusive)

    def filter(self, selector):
        """Select all elements that fit the specified condition.

        This is a Selector.

        There are a number of ways to specify a selector for
        :func:`filter`. The simplest way is to pass a dict that
        defines a JSON document:

        >>> table('users').filter( { 'age': 30, 'state': 'CA'}) # select all thirty year olds in california

        We can also pass ReQL expressions directly. The above query is
        equivalent to the following query:

        >>> table('users').filter((R('age') == 30) & (R('state') == 'CA')))

        The values in a dict can contain ReQL expressions - they will
        get evaluated in order to evaluate the condition:

        >>> # Select all Californians whose age is equal to the number
        >>> # of colleges attended added to the number of jobs held
        >>> table('users').filter( { 'state': 'CA', 'age': R('jobs_held') + R('colleges_attended') })

        We can of course specify this query as a ReQL expression directly:

        >>> table('users').filter(R('state') == 'CA' &
        >>>                       R('age') == R('jobs_held') + R('colleges_attended'))

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
        :type selector: dict, :class:`JSONExpression`
        :returns: :class:`Stream`, :class:`MultiRowSelection`, :class:`JSONExpression` (depends on input)
        """
        if isinstance(selector, dict):
            selector = internal.All(*[R(k) == v for k, v in selector.iteritems()])
        if not isinstance(selector, internal.JSONFunction):
            selector = internal.JSONFunction(selector)

        return internal.Filter(self, selector)

    def __getitem__(self, index):
        if isinstance(index, slice):
            if index.step is not None:
                raise ValueError("slice stepping is unsupported")
            return internal.Slice(self, index.start, index.stop)
        elif isinstance(index, int):
            return internal.Nth(self, index)
        else:
            if isinstance(self, JSONExpression):
                return internal.Attr(self, index)
            raise TypeError("stream indices must be integers, not " + index.__class__.__name__)

    def nth(self, index):
        """Select the element at `index`.

        .. note:: ``e.nth(index)`` is equivalent to ``e[index]``.

        This is a Selector.

        :param index: The element number to return.
        :type index: int
        :returns: :class:`RowSelection`, :class:`JSON` (depends on input)

        >>> expr([1, 2, 3, 4, 5]).nth(2)  # returns 3
        >>> table('users').nth(10) # returns 11th row
        >>> table('users')[10]     # returns 11th row
        """
        return self[index]

    def skip(self, offset):
        """Skip elements before the element at `offset`.

        .. note:: ``e.skip(offset)`` is equivalent to ``e[offset:]``.

        This is a Selector.

        :param offset: The number of elements to skip.
        :type offset: int
        :returns: :class:`Stream`, :class:`MultiRowSelection`, :class:`JSONExpression` (depends on input)

        """
        return self[offset:]

    def limit(self, count):
        """Select elements before the element at `count`.

        .. note:: ``e.limit(count)`` is equivalent to ``e[:count]``.

        This is a Selector.

        :param count: The number of elements to select.
        :type count: int
        :returns: :class:`Stream`, :class:`MultiRowSelection`, :class:`JSONExpression` (depends on input)
        """
        return self[:count]

    def orderby(self, *ordering):
        """Sort elements according to attributes specified by strings.

        Items are sorted in ascending order unless the attribute name starts
        with '-', which sorts the attribute in descending order.

        This is a Selector.

        :param ordering: attribute names to order by
        :type ordering: list(str)
        :returns: :class:`Stream`, :class:`MultiRowSelection`, :class:`JSONExpression` (depends on input)

        >>> table('users').orderby('name')  # order users by name A-Z
        >>> table('users').orderby('-level', 'name') # levels high-low, then names A-Z
        """
        order = []
        for attr in ordering:
            if attr.startswith('-'):
                order.append((attr[1:], False))
            else:
                order.append((attr, True))
        return internal.OrderBy(self, order)

    def random(self):
        """Select a random element.

        This is a Selector.

        :returns: :class:`Stream`, :class:`MultiRowSelection`, :class:`JSONExpression` (depends on input)
        """
        raise NotImplementedError

    def sample(self, count):
        """Select `count` elements at random.

        This is a Selector.

        :returns: :class:`Stream`, :class:`MultiRowSelection`, :class:`JSONExpression` (depends on input)
        """
        raise NotImplementedError

    def map(self, mapping):
        """Evaluate `mapping` for each element, with the implicit
        variable containing the element if mapping does not bind it.

        :param mapping: The expression to evaluate
        :type mapping: :class:`JSONExpression`
        :returns: :class:`Stream`, :class:`JSONExpression` (depends on input)

        >>> expr([1, 2, 3]).map(R('@') * 2) # gives JSONExpression evaluating to [2, 4, 6]
        >>> table('users').map(R('age'))
        >>> table('users').map(fn('user', table('posts').filter({'userid': R('$user.id')})))
        """
        if not isinstance(mapping, internal.JSONFunction):
            mapping = internal.JSONFunction(mapping)
        return internal.Map(self, mapping)

    def concat_map(self, mapping):
        """Evaluate `mapping` for each element. The result of `mapping` must be
        a stream; all those streams will be concatenated to produce the result
        of `concat_map()`.

        :param mapping: The mapping to evaluate
        :type mapping: :class:`Stream`   # TODO fixme
        :returns: :class:`Stream`

        >>> expr([1, 2, 3]).concat_map(fn("a", expr([R("$a"), "test"]).to_stream())).run()
        [1, "test", 2, "test", 3, "test"]
        """
        if not isinstance(mapping, internal.StreamFunction):
            mapping = internal.StreamFunction(mapping)
        return internal.ConcatMap(self, mapping)

    def reduce(self, base, func):
        """Build up a result by repeatedly applying `func` to pairs of elements. Returns
        `base` if there are no elements.

        `base` should be an identity (`func(base, e) == e`).

        :type base: :class:`JSONFunction`, :class:`JSON`
        :returns: :class:`Stream`, :class:`MultiRowSelection`, :class:`JSONExpression` (depends on input)

        >>> expr([1, 2, 3]).reduce(0, fn('a', 'b', R('$a') + R('$b'))).run()
        6

        >>> table('users').reduce(0, fn('a', 'b', R('$a') + R('$b.credits)))
        """
        raise NotImplementedError

    def distinct(self, *attrs):
        """Select distinct elements of the input.

        If attribute names are passed, it will first pluck the given attributes
        and then limit to distinct elements.

        :param attrs: The attributes to find distinct combinations of.
        :type attrs: str
        :returns: :class:`Stream`, class `JSONExpression` (depends on input)
        """
        if attrs:
            return self.pluck(*attrs).distinct()
        return internal.Distinct(self)

    def pluck(self, attr, *attrs):
        if not attrs:
            return self.map(internal.ImplicitAttr(attr))
        else:
            return self.map([internal.ImplicitAttr(x) for x in (attr,) + attrs])

##########################################
# DATA OBJECTS - SELECTION, STREAM, ETC. #
##########################################
class Stream(Expression):
    """A sequence of JSON values which can be read."""
    def to_array(self):
        """Convert the stream into a JSON array."""
        return internal.ToArray(self)

    def count(self):
        return internal.Count(self)

class BaseSelection(object):
    """Something which can be read or written."""
    def delete(self):
        """Delete all rows in the selection from the database."""
        return internal.Delete(self)

    def update(self, mapping):
        """Update all rows in the selection by merging the current contents
        with the value of `mapping`.

        The merge is recursive, see :

        >>> table('users').filter(R('warnings') > 5).update({'banned': True})

        """
        if not isinstance(mapping, internal.JSONFunction):
            mapping = internal.JSONFunction(mapping)
        return internal.Update(self, mapping)

    def mutate(self, mapping):
        """TODO: get rid of this ?"""
        if not isinstance(mapping, internal.JSONFunction):
            mapping = internal.JSONFunction(mapping)
        return internal.Mutate(self, mapping)

class MultiRowSelection(Stream, BaseSelection):
    """A sequence of rows which can be read or written."""

class JSONExpression(Expression):
    """A JSON value.

    Use :func:`expr` to create expressions for JSON values.

    >>> expr(1)      # returns :class:`JSONExpression` that encodes JSON value 1.
    >>> expr([1, 2]) # returns :class:`JSONExpression` that encodes a JSON array.
    >>> expr("foo")  # returns :class:`JSONExpression` that encodes a JSON string.
    >>> expr({ 'name': 'Joe', 'age': 30 }) # returns :class:`JSONExpression` that encodes a JSON object.

    Python operators enable comparisons, logic, and algebra to be performed
    on JSON expressions.

    >>> conn.run(expr(1) < expr(2)) # Evaluates 1 < 2 on the server and returns True.
    >>> expr(1) < 2 # Whenever possible, ReQL converts Python types to expressions implicitly.
    >>> expr(1) + 2 # Addition. We can do `-`, `*`, `/`, and `%` in the same way.
    >>> expr(1) < 2 # We can also do `>`, `<=`, `>=`, `==`, etc.
    >>> expr(1) < 2 & 3 < 4 # We use `&` and `|` to encode `and` and `or` since we can't overload
                            # these in Python"""

    def to_stream(self):
        """Convert a JSON array into a stream."""
        return internal.ToStream(self)

    def __lt__(self, other):
        return internal.CompareLT(self, other)
    def __le__(self, other):
        return internal.CompareLE(self, other)
    def __eq__(self, other):
        return internal.CompareEQ(self, other)
    def __ne__(self, other):
        return internal.CompareNE(self, other)
    def __gt__(self, other):
        return internal.CompareGT(self, other)
    def __ge__(self, other):
        return internal.CompareGE(self, other)

    def __add__(self, other):
        return internal.Add(self, other)
    def __sub__(self, other):
        return internal.Sub(self, other)
    def __mul__(self, other):
        return internal.Mul(self, other)
    def __div__(self, other):
        return internal.Div(self, other)
    def __mod__(self, other):
        return internal.Mod(self, other)

    def __radd__(self, other):
        return internal.Add(other, self)
    def __rsub__(self, other):
        return internal.Sub(other, self)
    def __rmul__(self, other):
        return internal.Mul(other, self)
    def __rdiv__(self, other):
        return internal.Div(other, self)
    def __rmod__(self, other):
        return internal.Mod(other, self)

    def __neg__(self):
        return internal.Sub(self)
    def __pos__(self):
        return self

class JSONLiteral(JSONExpression):
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
                expr(value)._write_ast(parent.array.add())
        elif self.type == p.Term.OBJECT:
            for key, value in self.value.iteritems():
                pair = parent.object.add()
                pair.var = key
                expr(value)._write_ast(pair.term)
        elif self.value is not None:
            setattr(parent, self.valueattr, self.value)

def expr(val):
    """Converts a python value to a ReQL :class:`JSONExpression`.

    :param val: Any Python value that can be converted to JSON.
    :returns: :class:`JSON`

    >>> expr(1)
    >>> expr("foo")
    >>> expr(["foo", 1])
    >>> expr({ 'name': 'Joe', 'age': 30 })
    """
    if isinstance(val, JSONExpression):
        return val
    return JSONLiteral(val)

class RowSelection(JSONExpression, BaseSelection):
    """A single row from a table which can be read or written."""

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
    raise NotImplementedError

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
    raise NotImplementedError

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
    raise NotImplementedError

##############################################
# LEAF SELECTORS - DATABASE AND TABLE ACCESS #
##############################################
class Database(object):
    """A ReQL expression that encodes a RethinkDB database. Most
    database-related operations (including table access) can be
    chained off of this object."""
    def __init__(self, db_name):
        """Use :func:`rethinkdb.query.db` to create this object.

        :param db_name: Name of the databases to access.
        :type db_name: str
        """
        self.db_name = db_name

    def create(self, table_name, primary_key=None):
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
        :returns: :class:`JSONExpression` -- a ReQL expression that
          encodes the table creation operation.

        :Example:

        >>> q = db('db_name').create('posts') # uses primary key 'id'
        >>> q = db('db_name').create('users', primary_key='user_id')
        """
        raise NotImplementedError

    def drop(self, table_name):
        """Create a ReQL expression that drops a table within this
        RethinkDB database.

        When run via :func:`rethinkdb.net.Connection.run` or
        :func:`Expression.run`, `run` has no return value in case of
        success, and raises :class:`rethinkdb.net.QueryError` in case
        of failure.

        :param table_name: The name of the table to be dropped.
        :type table_name: str
        :returns: :class:`JSONExpression` -- a ReQL expression that
          encodes the table creation operation.

        :Example:

        >>> q = db('db_name').drop('posts')
        """
        raise NotImplementedError

    def list(self):
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
        raise NotImplementedError

    def table(self, table_name):
        """Create a ReQL expression that encodes a table within this
        RethinkDB database. This function is a shortcut for
        constructing the :class:`Table` object.

        Use :func:`rethinkdb.query.table` as a shortcut for this
        method.

        :returns: :class:`Table` -- a ReQL expression that encodes the
          table expression.
        """
        return Table(table_name, self)

def db(db_name):
    """Create a ReQL expression that encodes a database within a
    RethinkDB cluster. This function is a shortcut for constructing
    the :class:`Database` object.

    :returns: :class:`Database` -- a ReQL expression that encodes the
      database expression.

    :Example:

    >>> q = db('db_name')
    """
    return Database(db_name)

class Table(MultiRowSelection):
    """A ReQL expression that encodes a RethinkDB table. Most data
    manipulation operations (such as inserting, selecting, and
    updating data) can be chained off of this object."""

    def __init__(self, table_name, db_expr=None):
        """Use :func:`rethinkdb.query.table` as a shortcut to create
        this object.

        :param table_name: Name of the databases to access.
        :type table_name: str
        :param db_expr: An optional database where this table
          resides. If missing, use default database specified on the
          connection object.
        :type db_expr: :class:`Database`
        """
        self.table_name = table_name
        self.db_expr = db_expr

    def insert(self, docs):
        """Insert documents into the table.

        :param docs: the document(s) to insert
        :type docs: dict/list(dict)
        """
        if isinstance(docs, dict):
            return internal.Insert(self, [docs])
        else:
            return internal.Insert(self, docs)

    def insert_stream(self, stream):
        return internal.InsertStream(self, stream)

    def get(self, key):
        """Select a row by primary key. If the key doesn't exist, returns null.

        :param key: the key to look for
        :type key: JSON value
        :returns: :class:`RowSelection`, :class:`JSONExpression`

        >>> q = table('users').get(10)  # get user with primary key 10
        """
        return internal.Get(self, key)

    def _write_ref_ast(self, parent):
        parent.db_name = self.db_expr.db_name
        parent.table_name = self.table_name

    def _write_ast(self, parent):
        parent.type = p.Term.TABLE
        self._write_ref_ast(parent.table.table_ref)

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
    if '.' in table_ref:
        db_name, table_name = table_ref.split('.', 1)
        return db(db_name).table(table_name)
    else:
        return Table(table_ref)


# this happens at the end since it's a circular import
import internal
