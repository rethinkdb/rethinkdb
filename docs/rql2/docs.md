# Accessing the ReQL API.

**Basic Example:**

<PY COMPLETE_EXAMPLE>
    import rethinkdb as r

    # Open a connection.
    conn = r.connect('localhost', 28015, 'test')

    # Create a table and insert three rows.
    r.table_create('tristate_area').run(conn)
    r.table('tristate_area').insert([{'name': 'Pennsylvania', 'code': 'PA'},
                                     {'name': 'New Jersey', 'code': 'NJ'},
                                     {'name': 'Delaware', 'code': 'DE'}]).run(conn)

    # Make a cursor containing every row of the table and destructively
    # iterate through its rows.
    cursor = r.table('tristate_area').run(conn)
    for row in cursor:
        print row

    # Cleanup.  (Exception safety is not a part of this example!)
    r.table_drop('tristate_area').run(conn)
    conn.close()
<RB COMPLETE_EXAMPLE>
    # Load the RethinkDB module.
    require 'rethinkdb'

    # Make the RethinkDB API accessible by the name 'r'.
    include RethinkDB::Shortcuts

    # Open a connection.
    conn = r.connect('localhost', 28015, 'test')

    # Create a table and insert three rows.
    r.table_create('tristate_area').run(conn)
    r.table('tristate_area').insert([{:name => "Pennsylvania", :code => 'PA'},
                                     {:name => 'New Jersey', :code => 'NJ'},
                                     {:name => 'Delaware', :code => 'DE'}]).run(conn)

    # Make a cursor containing every row of the table and destructively
    # iterate through its rows.
    cursor = r.table('tristate_area').run(conn)
    cursor.each { |x|
      puts x
    }

    # Cleanup.  (Exception safety is not a part of this example!)
    r.table_drop('tristate_area').run(conn)
    conn.close()
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

<JS>
    var r = require('rethinkdb');
<PY>
    import rethinkdb as r
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

<PY COMPLETE_EXAMPLE>
    import rethinkdb as r

    # A fresh RethinkDB instance comes with an empty database named
    # 'test' created for you.

    # Open a connection with 'test' as the default database.
    conn = r.connect('localhost', 28015, 'test')

    # Create a second database, 'test_two'.
    r.db_create('test_two').run(conn)

    print "Databases: ", r.db_list().run(conn)

    # Create a table in conn's default database, 'test'.
    r.table_create('alice').run(conn)

    # Create another table, explicitly specifying the database 'test'.
    r.db('test').table_create('bob').run(conn)

    # Create a third table, explicitly specifynig the database 'test_two'.
    r.db('test_two').table_create('charlie').run(conn)

    # Print the tables of conn's default database, 'test'.
    print "Tables in 'test': ", r.table_list().run(conn)
    # Print the tables of 'test_two'.
    print "Tables in 'test_two': ", r.db('test_two').table_list().run(conn)

    # Clean up our work.  (This example code is not exception-safe.)

    # Drop the database 'test_two' and ALL of its tables.
    r.db_drop('test_two').run(conn)

    # Drop the tables 'alice' and 'bob' from conn's default database,
    # which is 'test'.
    r.table_drop('alice').run(conn)
    r.table_drop('bob').run(conn)

    conn.close()
<RB COMPLETE_EXAMPLE>
    require 'rethinkdb'
    include RethinkDB::Shortcuts

    # A fresh RethinkDB instance comes with an empty database named
    # 'test' created for you.

    # Open a connection with 'test' as the default database.
    conn = r.connect('localhost', 28015, 'test')

    # Create a second database, 'test_two'.
    r.db_create('test_two').run(conn)

    puts "Databases:", r.db_list().run(conn)

    # Create a table in conn's default database, 'test'.
    r.table_create('alice').run(conn)

    # Create another table, explicitly specifying the database 'test'.
    r.db('test').table_create('bob').run(conn)

    # Create a third table, explicitly specifynig the database 'test_two'.
    r.db('test_two').table_create('charlie').run(conn)

    # Print the tables of conn's default database, 'test'.
    puts "Tables in 'test':", r.table_list().run(conn)
    # Print the tables of 'test_two'.
    puts "Tables in 'test_two':", r.db('test_two').table_list().run(conn)

    # Clean up our work.  (This example code is not exception-safe.)

    # Drop the database 'test_two' and ALL of its tables.
    r.db_drop('test_two').run(conn)

    # Drop the tables 'alice' and 'bob' from conn's default database,
    # which is 'test'.
    r.table_drop('alice').run(conn)
    r.table_drop('bob').run(conn)

    conn.close()
<JS> TODO
</>

<PY||RB>
## repl
### connection.repl()

Set the default connection to make REPL use easier.  Allows calling
`run()` without specifying a connection.

Connection objects are not thread safe, and `repl` connections should
not be used in multi-threaded environments.  (The use of global
variables in single-threaded environments is also questionable.)

<PY COMPLETE_EXAMPLE>
    import rethinkdb as r

    # Open a connection to localhost:28015, with 'test' as the default
    # database.
    conn = r.connect()

    # We must explicitly pass conn.
    print "Tables:", r.table_list().run(conn)

    # Start "repl" mode, which is useful for irb and small scripts.
    conn.repl()

    # Now we don't need to explicitly pass conn.  conn.repl() put conn in
    # a global variable that run defaults to using.
    print "Tables:", r.table_list().run()
<RB COMPLETE_EXAMPLE>
    require 'rethinkdb'
    include RethinkDB::Shortcuts

    # Open a connection to localhost:28015, with 'test' as the default
    # database.
    conn = r.connect()

    # We must explicitly pass conn.
    puts "Tables: [" + r.table_list().run(conn).join(', ') + "]"

    # Start "repl" mode, which is useful for irb and small scripts.
    conn.repl()

    # Now we don't need to explicitly pass conn.  conn.repl() put conn in
    # a global variable that the 'run' method uses by default.
    puts "Tables: [" + r.table_list().run.join(', ') + "]"
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
  `<False>`.  This can be fine-tuned precisely by using the
  `use_outdated` option of `r.table`, which takes overrides this
  precedence over this option.
* `noreply`: Don't wait for a reply, and return `nil`.  Defaults to
  `<False>`.  This allows multiple write queries to be sent without
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
                console.log(["Row " + i + ":", row]);
                i += 1;
                if (!cursor.hasNext()) {
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
  `<True>`, specifies that writes to this table
  should be considered completed only when they're reliably stored on
  disk.  Otherwise, writes are considered completed when they're
  stored in the cache.  Defaults to `<True>`.

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
                              'warnings': 0, 'access': 'moderator'},
                             {'username': 'Mozart', 'postCount': 3,
                              'warnings': 17, 'access': 'banned'},
                             {'username': 'Liszt', 'postCount': 123,
                              'warnings': 3, 'access': 'normal'},
                             {'username': 'Dickens', 'postCount': 0,
                              'warnings': 5, 'access': 'normal'},
                             {'username': 'Hugo', 'postCount': 0,
                              'warnings': 0, 'access': 'normal'}]).run(conn)

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
    r.table('users').insert([{:username => 'Beethoven', :postCount => 37,
                              :warnings => 0, :access => 'moderator'},
                             {:username => 'Mozart', :postCount => 3,
                              :warnings => 17, :access => 'banned'},
                             {:username => 'Liszt', :postCount => 123,
                              :warnings => 3, :access => 'normal'},
                             {:username => 'Dickens', :postCount => 0,
                              :warnings => 0, :access => 'normal'}]).run(conn)

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

## insert
<PY>
### table.insert(json, upsert=False) -> query
### table.insert([json], upsert=False) -> query
TODO: How to represent an array of JSON?
<RB||JS>
### table.insert(json) -> query
### table.insert(json, options) -> query
### table.insert([json]) -> query
### table.insert([json], options) -> query
</>

Inserts JSON documenst into a table.  Acceps a single JSON document or
an array of documents.  If `table`'s primary key column is not
supplied, it will be autogenerated.  If a document already exists with
the given primary key, the document won't get inserted, unless
the optional argument `upsert` is specified to be <True>.

TODO: Above: Link to primary key autogeneration doc.

Options:

* `json`: A JSON document or an array of JSON documents.  The JSON
  document (or documents) to insert.

<RB||JS>More options can be supplied in the `options` dictionary.</>

* `upsert`: A boolean.  If set to `<True>`, the operation will
  overwrite documents with a matching primary key.  Defaults to
  `<False>`.

When evaluated:  Returns an object that contains the following attributes:

* `inserted`: the number of documents that were successfully inserted.

* `replaced`: the number of odcuments that were updated when `upsert` is used.

* `unchanged`: the number of documents that would have been modified,
  except that the new value was the same as the old value when doing
  an `upsert`.

* `errors`: the number of errors encountered while inserting,

* If errors were encountered while inserting, `first_error` contains
  the text of the first error,

* `generated_keys`: a list of generated primary key values,

TODO: Is `generated_keys` in the same order as the insertions?

* `deleted`: the number of documents deleted (which is always `0` for
  an `insert` operation).

* `skipped`: the number of documents skipped (which is always `0` for
  an `insert` operation).

**Example:**

<PY>
    # Connect and create a new table 'points'.
    conn = r.connect()
    r.table_create('points').run(conn)

    # Insert one document.
    results = r.table('points').insert({'x': 3, 'y': 4}).run(conn)

    # Insert two more documents.
    results = r.table('points').insert([{'x': 5, 'y': 12},
                                        {'x': 7, 'y': 24}]).run(conn)
    ids = results.generated_keys

    print "Freshly inserted documents:"
    for row in r.table('points').run(conn)
        print row

    # Overwrite the document {'x': 5, 'y': 12}.  TODO: Check that
    # results.generated_keys must retain order.  Explain this here.
    r.table('points')
     .insert({'id': ids[0], 'x': 3, 'y': 4}, upsert=True).run(conn)

    print "Slightly different documents:"
    for row in r.table('points').run(conn)
        print row
<RB>
    # Connect and create a new table 'points'.
    conn = r.connect()
    r.table_create('points').run(conn)

    # Insert one document.
    results = r.table('points').insert({ :x => 3, :y => 4}).run(conn)

    # Insert two more documents.
    results = r.table('points').insert([{ :x => 5, :y => 12 },
                                        { :x => 7, :y => 24 }]).run(conn)
    ids = results.generated_keys

    print "Freshly inserted documents:"
    for row in r.table('points').run(conn)
        print row

    # Overwrite the document { :x => 5, :y => 12}.  TODO: Check that
    # results.generated_keys must retain order.  Explain this here.
    r.table('points')
     .insert({'id': ids[0], 'x': 20, 'y': 21}, :upsert => true).run(conn)

    print "Slightly different documents:"
    for row in r.table('points').run(conn)
        print row
<JS>
    TODO
<CLEANUP PY>
    r.table_drop('points').run()
</>

## update
<PY>
### selection.update(expr, non_atomic=False) -> query
<RB>
### selection.update { expr } -> query
### selection.update { |row| expr } -> query
### selection.update(:non_atomic) { expr } -> query  (TODO: is this :non_atomic syntax good?)
### selection.update(:non_atomic) { |row| expr } -> query
<JS>
### selection.update(expr[, options]) -> query
</>

Update documents, column-by-column, in a selection.  The first step is
to write a query that selects the document or documents you'd like to
update.  This produces `selection`.  Then, there's the question of how
you'd like to update the documents.  If you provide a JSON object,
you'll update just the specific columns used in that dictionary.  You
can also compute this JSON dictionary by providing a function of the
row you're operating on.

Options:

* `expr`: TODO

<RB||JS> Further options are passed as part of the `options` dictionary.</>

* `<N>non_atomic</>`: Update the row non-atomically.  This flag lets
  you use non-deterministic and non-atomic query expressions, such as
  those that retrieve data from other tables, or those that use
  `r.js(...)` to evaluate Javascript on the server.  Defaults to
  `False`.

**Example:** Suppose we're a forum administrator that would like to
add `" (Moderator)"` to all moderators' user names, set their warning
level to 0, and randomize their post count.

<SETUP PY>
    r.table_create('users').run()
    r.table('users').insert([{'username': 'Beethoven', 'postCount': 37,
                              'warnings': 0, 'access': 'moderator'},
                             {'username': 'Mozart', 'postCount': 3,
                              'warnings': 17, 'access': 'banned'},
                             {'username': 'Liszt', 'postCount': 123,
                              'warnings': 3, 'access': 'normal'},
                             {'username': 'Dickens', 'postCount': 0,
                              'warnings': 5, 'access': 'normal'},
                             {'username': 'Hugo', 'postCount': 0,
                              'warnings': 0, 'access': 'normal'}]).run()
<PY>
    # Construct (but don't run) a ReQL query that selects moderators
    # in the `users` table.
    selection = r.table('users').filter({'access': 'moderator'})

    # Set moderators' warnings to 0.
    selection.update({'warnings': 0}).run(conn)

    # Update moderators' user names.
    selection.update(lambda x: {'username': x['username'] + ' (Moderator)'}).run(conn)

    # Randomize moderators' post count.  Since ReQL doesn't provide a
    # random number function, we'll have to use Javascript.  We'll
    # also have to declare the query non-atomic.

    javascript_expression = '(Math.floor(Math.random() * 4000))'
    selection.update(lambda x: {'postCount': r.js(javascript_expression)},
                     non_atomic=True)
             .run(conn)
<RB>
    # Construct (but don't run) a ReQL query that selects moderators
    # in the `users` table.
    selection = r.table('users').filter({ :access => 'moderator' })

    # Set moderators' warnings to 0.
    selection.update { { :warnings => 0 } }.run(conn)

    # Update moderators' user names.
    selection.update { |x| {:username => x[:username] + ' (Moderator)'} }.run(conn)

    # Randomize moderators' post count.
    javascript_expression = '(Math.floor(Math.random() * 4000))'
    selection.update(:non_atomic) { |x|
      {:postCount => r.js(javascript_expression)}
    }.run(conn)
<JS>
    TODO
<CLEANUP PY>
    r.table_drop('users').run()
</>

## replace
<PY>
### selection.replace(expr, non_atomic=False) -> query
<RB>
### selection.replace { expr } -> query
### selection.replace { |row| expr } -> query
### selection.replace(:non_atomic) { expr } -> query
### selection.replace(:non_atomic) { |row| expr } -> query
<JS>
### selection.replace(expr[, options]) -> query
</>

Replace documents found in a selection.  The first step is to write a
query that selects the document or documents you'd like to update.
This produces `selection`.  Then, there's the question of how you'd
like to replace the documents.  If you provide a JSON object, you'll
be directly providing the replacement value for that document.  If you
provide a ReQL expression, or function that produces a ReQL
expression, the replacement value will be computed from the row that's
getting replaced.  If the new document specifies the primary key
value, it must be the same value as the original document's primary
key.

(TODO: Must we specify the new document's primary key value?)

Options:

* `expr`: TODO

<RB||JS> Further options are passed as part of the `options` dictionary.</>

* `<N>non_atomic</>`: Update the row non-atomically.  This flag lets
  you use non-deterministic and non-atomic query expressions, such as
  those that retrive data from other tables, or those that use
  `r.js(...)` to evaluate Javascript on the server.  Defaults to
  `False`.

(TODO: Continue with replace examples.)