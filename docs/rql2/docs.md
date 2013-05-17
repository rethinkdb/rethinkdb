# Accessing the RQL API.

**Basic Example:**

<PY_EXAMPLE_WITH_OUTPUT examples/basic_example.py>
<RB_EXAMPLE_WITH_OUTPUT examples/basic_example.rb>
<JS> TODO
</>

## Importing the `rethinkdb` module as `r`

We recommend making the API accessible via the module name `r`.  All
examples will assume the module is imported as such.

<RB>After importing the `rethinkdb` module, use `include
RethinkDB::Shortcuts` to make RethinkDB API functions available this
way.
</>

**Syntax:**

    <JS> var r = require('rethinkdb');
    <PY> import rethinkdb as r
    <RB>
    require 'rethinkdb'
    include RethinkDB::Shortcuts
    </>

## connect

<JS> ### r.connect(options, function(err, conn) {})  // TODO: What is the type of err?
<PY> ### r.connect(host='localhost', port=28015, db='test') -> connection
<RB> ### r.connect(host='localhost', port=28015, default_db='test') -> connection
</>

Open a connection to a RethinkDB server.

TODO:  Is default_db up-to-date documentation?

<JS>`options` must be an object.</>

Options:
* `host`: A domain name or IP address of the server to connect to.  Defaults to `'localhost'`.
* `port`: Port of remote server.  Defaults to `28015`.
* `<JS||PY>db<RB>default_db</>`: The default database to use for the connection when performing queries.  Defaults to `'test'`.

#

**Example:**

<PY_EXAMPLE_WITH_OUTPUT examples/connect_example.py>
<RB_EXAMPLE_WITH_OUTPUT examples/connect_example.py>
<JS> TODO
</>

<PY||RB>
## repl
<PY> ### connection.repl()
<RB> ### connection.repl()
<PY||RB>

Set the default connection to make REPL use easier.  Allows calling
`run()` without specifying a connection.

Connection objects are not thread safe, and `repl` connections should
not be used in multi-threaded environments.  (The use of global
variables in single-threaded environments is also questionable.)

<PY_EXAMPLE_SANS_OUTPUT examples/repl_example.py>
<RB_EXAMPLE_SANS_OUTPUT examples/repl_example.rb>

</>

## close
### connection.close()

Close an open connection.  Cancels all outstanding requests.

## reconnect
<PY> ### connection.reconnect()
<RB> ### connection.reconnect
<JS> ### connection.reconnect(function(err, conn) {})
</>

<JS> TODO: We pass conn to the callback?  Isn't it the same conn? </>

Close and attempt to reopen a connection.  Cancels all outstanding
requests on the previously open connection.



## use
### connection.use(<i>database</i>)

Change the default database for this connection.

* `database`: The name of the database to become `connection`'s
  default database.

**Example:**

<SETUP PY>
    r.db_create('reality').run()
    r.db_create('fiction').run()
    r.db('reality').table_create('states').run()
    r.db('fiction').table_create('states').run()
<PY WITH_OUTPUT>
    # Open a connection to localhost:28015 with the default database
    # set to 'reality'
    conn = r.connect(db='reality')

    # Insert a document into the table 'states' in the 'reality' database.
    r.table('states').insert({'name': 'New York', 'code': 'NY'}).run(conn)

    # Change the default database to 'fiction'.
    conn.use('fiction')

    # Insert a document into the entirely different table 'states' in
    # the 'fiction' database.
    r.table('states').insert({'name': 'Cascadia', 'code': 'CS'}).run(conn)
<RB WITH_OUTPUT>
    # Open a connection with the default database set to 'reality'
    conn = r.connect('localhost', 28015, 'reality')

    # Insert a document into the table 'states' in the 'reality' database.
    r.table('states').insert({:name => 'New York', :code => 'NY'}).run(conn)

    # Change the default database to 'fiction'.
    conn.use('fiction')

    # Insert a document into the entirely different table 'states' in
    # the 'fiction' database.
    r.table('states').insert({:name => 'Cascadia', :code => 'CS'}).run(conn)
<JS>
    TODO

<CLEANUP PY>
    r.db_drop('reality').run()
    r.db_drop('fiction').run()
</>

## run
<JS> ### query.run(conn, callback)  TODO what kind of callback?
<PY> ### query.run([conn], [use_outdated=False]) -> result  TODO what options?
<RB> ### query.run([conn], [options]) -> result
</>

Run a query on the server.

<JS> TODO How do you specify options like use_outdated or noreply?
<PY> TODO Does python support a noreply option?
<RB>
`options`, if supplied must be a dictionary of options.

Options:
* `use_outdated`: Tolerate outdated data in this query.  Defaults to
  `False`.  This can be fine-tuned precisely by using the
  `use_outdated` option of `r.table`, which takes overrides this
  precedence over this option.
* `noreply`: Don't wait for a reply, and return `nil`.  Defaults to
  `False`.  This allows multiple write queries to be sent without
  waiting for or receiving the result, which means throughput is less
  hindered by client/server latency or other bottlenecks.  TODO: Check
  that the return value will be nil.

</>

**Example:** Runs the `r.table('tristate_area')` query (which returns all
rows in the table 'tristate_area').

<RB>You can run `.each { |row| ... }` to iterate through a cursor.
See <TODO add Cursor documentation> for more ways to iterate through
cursors.</>

<SETUP PY>
    r.table_create('numbers').run()
<PY WITH_OUTPUT>
    conn = r.connect()

    # Run an insert query, with an informational return value.
    result = r.table('numbers').insert({'value': -1}).run(conn)
    print "Write result: ", result

    # Run fifty insert queries, without waiting for replies.
    for i in xrange(50):
        r.table('numbers').insert({'value': i}).run(conn, noreply=True)

    # Read back the numbers and print them out.
    cursor = r.table('numbers').run(conn)
    for row in cursor:
        print row
<RB WITH_OUTPUT>
    conn = r.connect()

    # Run an insert query, with an informational return value.
    result = r.table('numbers').insert({ :value => -1 }).run(conn)

    # Run fifty insert queries, without waiting for replies.
    50.times { |i|
        r.table('numbers').insert({ :value => i }).run(conn, :noreply => true)
    }

    # Read back the numbers and print them out.
    cursor = r.table('numbers').run(conn)
    cursor.each { |row|
        puts row
    }
<JS>
    TODO
<CLEANUP PY>
    r.table_drop('numbers').run()
</>

<JS>
## next
### cursor.next(function(error, row) {})

Consume the next element in the cursor.

<SETUP PY>
    r.table_create('numbers').run()
    r.table('numbers').insert([{'value': i} for i in xrange(-1, 50)]).run()
<JS>
    function log_rows(cursor, i, callback) {
        if (!cursor.hasNext()) {
            // TODO: Would a Node.js person actually schedule the
            // callback later?
            callback();
        } else {
            cursor.next(function(err, row) {
                // TODO: Error handling...
                console.log(["Row " + i + ":", row]);
                log_rows(cursor, i + 1, callback);
            });
        }
    }

    r.connect({host: 'localhost', port: 28015}, function(err, conn) {
        // TODO: Error handling.
        r.table('numbers').run(conn, function(err, cursor) {
            // TODO: Error handling.
            log_rows(cursor, 0, function() {
                conn.close();
            });
        });
    });
<CLEANUP PY>
    r.table_drop('numbers').run()
</>

<JS>
TODO: What if we call .next between a call to .next and its callback?
</>

<JS>
## hasNext
### cursor.hasNext() -> bool

Check if there are more elements in the cursor.  Returns `true` if
there are more elements.
</>

<JS>
## each
### cursor.each(function(err, row) {})

Iterate over the result set one element at a time.  The callback gets
called for each remaining row in the cursor.

<SETUP PY>
    r.table_create('numbers').run()
    r.table('numbers').insert([{'value': i} for i in xrange(-1, 50)]).run()
<JS>
    r.connect({host: 'localhost', port: 28015}, function(err, conn) {
        // TODO: Error handling.
        r.table('numbers').run(conn, function(err, cursor) {
            var i = 0;

            cursor.each(function(err, row) {
                console.log(["Row " + i + ":", row])
                i += 1;
                if (!row.hasNext()) {
                    conn.close();
                }
            });
        });
    });
<CLEANUP PY>
    r.table_drop('numbers').run()
</>

<JS>
## toArray
### cursor.toArray(function(err, resultsArray) {})

Read all the remaining results from the cursor and pass them in an
array to the callback.

<SETUP PY>
    r.table_create('numbers').run()
    r.table('numbers').insert([{'value': i} for i in xrange(-1, 50)]).run()
<JS>
    r.connect({host: 'localhost', port: 28015}, function(err, conn) {
        // TODO: Error handling.
        r.table('numbers').run(conn, function(err, cursor) {
            cursor.toArray(function(err, results) {
                for (var i in results) {
                    console.log(["Row " + i + ":", result[i]]);
                }
                conn.close();
            });
        });
    });
<CLEANUP PY>
    r.table_drop('numbers').run()
</>



# Manipulating Databases

## <N>db_create</>
### r.<N>db_create</>(<N>db_name</>) -> query

Creates a database, which is a collection of tables.

Options:
* `db_name`: The name of the database to be created.  It may contain
  letters, numbers, and underscores.

When evaluated: Returns an object whose value is `<PY>{'created':
1}<RB>{ :created => 1 }<JS>{created: 1}</>` if the database was
successfully created.  Otherwise, <PY||RB>a `RqlRuntimeError` exception is
thrown<JS>an error is returned to the callback (TODO what error?)</>.

## db_drop
### r.<N>db_drop</>(<N>db_name</>) -> query

Drops a database, including its entire collection of tables.

Options:
* `db_name`: The name of the database to be dropped.

When evaluated: Returns `<PY>{'dropped': 1}<RB>{ :dropped => 1
}<JS>{dropped: 1}</>` when successful.  If the specified database did
not exist, <PY||RB>a `RqlRuntimeError` exception is thrown<JS>an error
is returned to the callback (TODO what error?)</>.

## <N>db_list</>
### r.<N>db_list</>() -> query

List all database names in the system.

When evaluated:  Returns <PY>a list<RB||JS>an array</> of strings.

<PY WITH_OUTPUT>
    conn = r.connect()
    print "Databases: ", r.db_list.run(conn)
    print r.db_create('a_new_database').run(conn)
    print "Databases now: ", r.db_list.run(conn)
    print r.db_drop('a_new_database').run(conn)
    print "Databases finally: ", r.db_list.run(conn)
    conn.close()
<RB WITH_OUTPUT>
    conn = r.connect()
    puts "Databases: ", r.db_list.run(conn)
    puts r.db_create('a_new_database').run(conn)
    puts "Databases now: ", r.db_list.run(conn)
    puts r.db_drop('a_new_database').run(conn)
    puts "Databases finally: ", r.db_list.run(conn)
    conn.close()
<JS WITH_OUTPUT>
    TODO
</>

# Manipulating Tables

A RethinkDB table is a database table that contains JSON documents.

## <N>table_create</>
<PY>
### db.table_create(table_name, primary_key='id', hard_durability=True, cache_size=1024, datacenter=None) -> query
### r.table_create(table_name, primary_key='id', hard_durability=True, cache_size=1024, datacenter=None) -> query
<RB||JS>
### db.<N>table_create<RB||JS>(<N>table_name<RB||JS>[, options]) -> query
### r.<N>table_create<RB||JS>(<N>table_name<RB||JS>[, options]) -> query
</>

Create a table in the database `db`, or, in the case of
`r.<N>table_create</>`, in the connection's default database.

Options:

* `<N>table_name</>`: The name of the table to be created.  It may
  contain letters, numbers, and underscores.

<RB||JS>The following options are available in the `options` dictionary.</>

* `<N>primary_key</>`: A string. The name of the
  table's primary key.  This cannot be changed after the table is
  created (but you can migrate your data to a new table with a
  different primary key).  Defaults to `'id'`.

* `<N>hard_durability</>`: A boolean.  If
  `<PY>True<RB||JS>true</>`, specifies that writes to this table
  should be considered completed only when they're reliably stored on
  disk.  Otherwise, writes are considered completed when they're
  stored in the cache.  Defaults to `<PY>True<RB||JS>true</>`.

* `<N>cache_size</>`: *Deprecated* (TODO: Really?  Sam added this
  claim himself.).  An integer.  The amount of cache space allocated
  to the table, in megabytes.  Defaults to `1024`.

* `datacenter`: A string.  The name of the datacenter this table
  should be assigned to.  Its primary replica will be on this
  datacenter.

When evaluated: If successful, returns `<PY>{'created': 1}<RB>{
:created => 1}<JS>{created: 1}</>`.  If the specified table already
exists, <PY||RB>a `RqlRuntimeException` will be thrown<JS>an error
will be returned (TODO what error)</>.

**Example:**

<SETUP PY>
<PY WITH_OUTPUT>
    # Open a connection with 'test' being the default database.
    conn = r.connect('localhost', 28015, 'test')

    # Create table 'foo' in the 'test' database.
    print r.db('test').table_create('foo').run(conn)

    # Create table 'bar' in the 'test' database (which is the default
    # for this connection) with soft durability.
    print r.table_create('bar', hard_durability=False).run(conn)

    # List tables.
    print r.db('test').table_list().run(conn)

    conn.close()
<RB WITH_OUTPUT
    # Open a connection with 'test' being the default database.
    conn = r.connect('localhost', 28015, 'test')

    # Create table 'foo' in the 'test' database.
    puts r.db('test').table_create('foo').run(conn)

    # Create table 'bar' in the 'test' database (which is the default
    # for this connection) with soft durability.
    puts r.table_create('bar', :hard_durability => false).run(conn)

    # List tables.
    puts r.db('test').table_list().run(conn)

    conn.close()
<JS>
    TODO
<CLEANUP PY>
    r.table_drop('foo').run()
    r.table_drop('bar').run()
</>

## <N>table_drop</>
### db.<N>table_drop</>(<N>table_name</>) -> query
### r.<N>table_drop</>(<N>table_name</>) -> query

Drops a table, from the specified database or from the connection's
default database.

Options:

* `<N>table_name</>`: The name of the database to be dropped.

When evaluated: Returns `<PY>{'dropped': 1}<RB>{ :dropped => 1
}<JS>{dropped: 1}</>` when successful.  If the specified table did
not exist, <PY||RB>a `RqlRuntimeError` exception is thrown<JS>an error
is returned to the callback (TODO what error?)</>.

## <N>table_list</>
### db.<N>table_list</>() -> query
### r.<N>table_list</>() -> query

List all table names in the specified database, or in the case of
`r.tableList()`, in the default database.

When evaluated:  Returns <PY>a list<RB||JS>an array</> of strings.


## <N>index_create</>
### table.<N>index_create</>(<N>index_name</>[, <N>index_function</>]) -> query

Create a new secondary index on the table.

Options:

* `<N>index_name</>`: A string.  The name of the index, to be used
when performing secondary index queries.

* `<N>index_function</>`: A query function.  This is applied to each
  row in `table`, computing the value by which we compare rows.  If
  not supplied, we index the rows by the column named
  `<N>index_name</>`.  That is, passing the
  <RB>block<PY||JS>function</> `<RB>{ |row| row[index_name]
  }<PY>lambda row: row[index_name]<JS>function(row) { return
  row(indexName); }</>` is the same as omitting this parameter.

TODO: Make "query function" above a link to a description of the
concept of query function.

As with all functions used in query building, `<N>index_function</>`
is applied once on the client with a dummy argument, to build a
function expression that is evaluated on the server.

**Examples:**

Suppose we're writing a forum, and suppose our forum has a
users table, with columns `username`, `postCount`, `warnings`, `bans`,
and the default primary key column, `id`.

The forum administrator wants to be able to sort users by post count,
and in other circumstances, by how much of a trouble-maker the user
is.

<SETUP PY>
<PY WITH_OUTPUT>
    # Set up some data to play with.
    r.table_create('users').run(conn)
    r.table('users').insert([{'username': 'Beethoven', 'postCount': 37,
                              'warnings': 0},
                             {'username': 'Mozart', 'postCount': 3,
                              'warnings': 17},
                             {'username': 'Liszt', 'postCount': 123,
                              'warnings': 3},
                             {'username': 'Dickens', 'postCount': 0,
                              'warnings': 5},
                             {'username': 'Hugo', 'postCount': 0,
                              'warnings': 0}]).run(conn)

    # Create an index by post count.
    r.table('users').index_create('postCount').run(conn)

    # Create another index, ordered by the warning/postCount ratio.
    r.table('users')
     .index_create('troublemaking',
                   lambda row: row['warnings'] / row['postCount'])
     .run(conn)
    # TODO: What happens when we get a divide-by-zero error?

    print "Our secondary indexes are: " + r.table('users').index_list().run()

    # Display some results within a certain contrived range of post counts.
    for row in r.table('users').between(2, 50, index='postCount').run(conn):
        print 'The post count of ' + row['username'] + ' is ' + row['postCount']

    # Display forum members, starting with the biggest trouble-makers.
    for row in r.table('users').order_by(r.desc('troublemaking')).run(conn):
        print 'Forum member ' + row['username'] + ' has ' + row['warnings'] +
            ' warnings after ' + row['postCount'] + ' posts.'

    # TODO: ^ Do we actually support order_by with indexes?

<RB WITH_OUTPUT>
    # Set up some data to play with.
    r.table_create('users').run(conn)
    r.table('users').insert([{:username => 'Beethoven', :postCount => 37},
                             {:username => 'Mozart', :postCount => 3},
                             {:username => 'Liszt', :postCount => 123},
                             {:username => 'Dickens', :postCount => 0}]).run(conn)

    # Create an index by post count.
    r.table('users').index_create('postCount').run(conn)

    # Query for results by that index.
    r.table('users').between(2, 50, index='postCount').run(conn).each { |row|
        print 'The post count of ' + row['username'] + ' is ' + row['postCount']
    }

    # Create another index, ordered by the warning/postCount ratio.
    r.table('users')
     .index_create('troublemaking',
                   lambda row: row['warnings'] / row['postCount'])
     .run(conn)
    # TODO: What happens when we get a divide-by-zero error?

    puts "Our secondary indexes are: " + r.table('users').index_list().run()

    # Display some results within a certain contrived range of post counts.
    r.table('users').between(2, 50, index='postCount').run(conn).each { |row|
        puts 'The post count of ' + row['username'] + ' is ' + row['postCount']
    }

    # Display forum members, starting with the biggest trouble-makers.
    r.table('users').order_by(r.desc('troublemaking')).run(conn).each { |row|
        puts 'Forum member ' + row['username'] + ' has ' + row['warnings'] +
            ' warnings after ' + row['postCount'] + ' posts.'
    }

    # TODO: ^ Do we actually support order_by with indexes?
<JS>
    TODO
<CLEANUP PY>
    r.table_drop('users').run()
</>

## <N>index_drop</>
### table.<N>index_drop</>(<N>index_name</>) -> query

Drops a secondary index from `table`.

Options:

* `<N>index_name</>`: A string.  The name of the index to drop.

When evaluated: TODO: What return value do we get?

**Example:**

<SETUP PY>
    r.table_create('users').run()
    r.table('users').index_create('troublemakers').run()
<PY>
    conn = r.connect()
    r.table('users').index_drop('troublemakers').run(conn)
<RB>
    conn = r.connect()
    r.table('users').index_drop('troublemakers').run(conn)
<JS>
    TODO
<CLEANUP PY>
    r.table_drop('users').run()
</>

## <N>index_list</>
### table.<N>index_list</>() -> query

Lists the secondary indexes of this table.

When evaluated:  Returns an array of strings.  TODO: Is this true?



# Writing Data

