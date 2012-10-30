# Copyright 2010-2012 RethinkDB, all rights reserved.

from rethinkdb import r

# Connections
conn = r.connect('newton', 5151)
conn = r.connect('newton')              # Default port
conn = r.connect()                      # Localhost, default port
conn = r.connect([('newton', 5151),
                  ('dr-doom', 5252)])   # A list in case of failure
  # Note: reconnect automatically or force them to call connect again?

# Get the entire table (the following queries are equivalent)
q = r.db('foo').table('bar')
q = r.table('foo.bar')

# Default database
conn = r.connect('newton', db='foo')
q = r.table('bar').run(conn)

conn.run(q)       # Either throws an error, or returns a Python iterator (or an element, see below)
# => iterator or scalar
conn.run([q1, q2, q3]) # Packing queries, asynchronisity of queries?
# => [a1, a2, a3]

# We can also start a query off a connection
r.db('foo').table('bar').run(conn)
q.run(conn)

# Default connection mode
r.set_default_conn(conn)
q.run()
q1.run()

# Specifying batch count/streaming
conn.run(q, batch_size=200) # Asyncrhonous preload, though we're not going to do that.

# Filter/selectors (the following three queries are equivalent)
q = r.table('foo.bar').filter({ 'age' : r.gt(r['candles']),
                                'name': 'Michel' })

q = r.table('foo.bar').filter(r.all(r.gt(r['age'], r['candles']),
                                    r.eq(r['name'], 'Michel')))

q = r.table('foo.bar').filter(r.all(r.gt(r['age'], r['candles'])),
                                    r.eq(r['name'], 'Michel')))

q = r.table('foo.bar').filter(r.all(r.gt(r.attr('age'),
                                         r.attr('candles')),
                                    r.eq(r.attr('name'), 'Michel')))

table('foo.bar').filter(R('age') > R('candles') & R('name') == 'Michel')

q = r.table('foo.bar').filter(r.eq(r['foo'], r.add(r['baz'], 5)))

# Get single row
q = r.table('foo.bar').get(5)              # Get row with primary key five
q = r.table('foo.bar').get(5, key='bar')   # Secondary unique indexes

# Get range of rows
q = r.table('foo.bar').range(5, 10)              # Get rows with primary key from five to ten
q = r.table('foo.bar').range(5, 10, key='bar')   # Secondary indexes

# Building  up queries off each other
q = r.table('foo.bar')
q1 = q.filter(...)
q2 = q.filter(...)
conn.run(q1)
conn.run(q2)

# More on chaining
q = r.table('foo.bar').filter(...)
                      .filter(...)   # That's ok

q = r.table('foo.bar').range(...)
                      .range(...)    # Error

q = r.table('foo.bar').range(...)
                      .filter(...)   # OK

# Order by
q = r.table('foo.bar').orderby('bar')               # Ascending order
q = r.table('foo.bar').orderby('bar', 'foo')        # Ascending, use foo as tie breaker (everyone says no list)
q = r.table('foo.bar').orderby(('bar', r.DESC),     # Tuples for order
                               ('foo', r.ASC))

q = r.table('foo.bar').orderby('bar')
                      .filter('blah')

q = r.table('foo.bar').filter('blah')
                      .orderby('bar', index_required=True)   # Throws an error if there is no index

q = r.table('foo.bar').sort(lambda r1, r2: r.lt(r.add(r1.foo, r2.baz),
                                                r2.bar))

q = r.table('foo.bar').sort_map(r.add(r['foo'], r['bar']))
q = r.table('foo.bar').sort_map(r.add(r['foo'], r['bar']), r.DESC) # optional order

# Inserts
q = r.table('foo.bar').insert({ 'foo': 1,
                                'bar': 2})       # Error out if primary key already exists,
                                                 # Returns generated primary keys
q = r.table('foo.bar').insert([{ 'foo': 1, 'bar': 2},
                               { 'foo': 3, 'bar': 4}])

q = r.table('foo.bar').filter.insert(...)    # Error

# Copy table
q = r.table('foo.bar').insert(r.db('foo.baz'))

# Inserts if stuff's missing, replaces full document otherwise
# (requires primary key). Returns nothing.
q = r.table('foo.bar').insert([{ 'foo': 1, 'bar': 2},
                               { 'foo': 3, 'bar': 4}],
                              upsert=True,
                              extend=True) # extend document

# Updates (multi row)
q = r.table('foo.bar').update({ 'foo': r.incr(r['foo']) })

q = r.table('foo.bar').filter({ 'key': 5 })
                      .update({ 'foo': r.incr(r['foo']) }) # Updates a subset (keeps all attributes)

q = r.table('foo.bar').range(10, 20)
                      .update({ 'foo': r.incr(r['foo']) }) # Updates a subset (keeps all attributes)

# Use return_data to have update return a stream (instead of a count)
q = r.table('foo.bar').range(10, 20)
                      .update({ 'foo': r.incr(r['foo']) },
                                  return_data=True)

# If we return data we still cannot chain
q = r.table('foo.bar').range(10, 20)
                      .update({ 'foo': r.incr(r['foo']) },
                              return_data=True)
                      .pluck('foo', 'bar')    # Error

# Update is also mutate -> returning null deletes the row (the
# following two are equivalent).
q = r.table('foo.bar').range(10, 20).update(r._if(r.eq(r['age'], 60),
                                                  when_true = { 'foo': r.undefined() },
                                                  when_else = { r.incr(r['salary'], 5000) }))

q = r.table('foo.bar').range(10, 20).filter(r.eq(r['age'], 60)).delete()
q = r.table('foo.bar').range(10, 20).filter(r.neq(r['age'], 60)).update(r.incr('salary', 5000))

q = r.table('foo.bar').range(10, 20).delete()

# Does nothing
q = r.table('foo.bar').range(10, 20).update({})

# Deletes
q = r.table('foo.bar').delete() # deletes every row
q = r.table('foo.bar').get(5).delete() # deletes row with PK 5
q = r.table('foo.bar').range(5, 10).delete() # deletes every row between 5 and 10

# Conditions
_if(r.eq(r['name'], 'Mike'),
    'Foo',
    'Bar')

_if(r.eq(r['name'], 'Mike'),
    when_true='Foo'
    when_false='Bar')

# Each (the following two are equivalent)
def foo(x):
    print "x"    # 'x' gets printed once
    return r.table('bar').insert(x)
q = r.table('foo.bar').range(10, 20).each(foo)
q = r.table('foo.bar').range(10, 20).each(lambda x: r.table('bar').insert(x))

# Distinct (gets all distinct names)
q = r.table('foo.bar').pluck('name').distinct()

# Javascript (whooo)
q = r.table('foo.bar').filter(r.js('this.age == 5'))

# Unions (everyone says no chaining)
q = r.union(r.table('foo.bar').range(10, 20),
            r.table('foo.bak').range(20, 30))

# Get the nth element
q = r.table('foo.bar').nth(50)

# Get the first N elements
q = r.table('foo.bar').limit(50)

# Count the number of rows
q = r.table('foo.bar').count()

# Get everything except the first fifty elements
q = r.table('foo.bar').skip(50)

# Map
q = r.table('foo.bar').range(10, 20).map({ 'name': r['name'],
                                           'foo': r['bar'] * r['bar'] })
q = r.table('foo.bar').range(10, 20).map( r['age'] )

# Reduce
q = r.table('foo.bar').range(10, 20).reduce(0, lambda x, y: r.add(x.age, y.age))

# Chaining map reduce
q = r.table('foo.bar').range(10, 20).map( r['age'] ).reduce(0, lambda x, y: r.add(x, y))

# Grouped map reduce (like Google and Hadoop, different from chaining map reduce)

# Throwing errors
q = r.table('foo.bar').range(10, 20).update(r._if(r.eq(r['age'], 60),
                                                  when_true = { 'foo': r.undefined() },
                                                  when_else = r.error('Everyone should be sixty!!')))

# Arrays
q = r.table('foo.bar').insert({ 'id': 1,
                                'bar': [1, 'foo', 'bar', 2]}) # Storing an array

q = r.table('foo.bar').get(1).update({ 'bar': r.append(r['bar'], 42) }) # Append 42 to an array

q = r.table('foo.bar').get(1).update({ 'bar': r.concat(r['bar'], r['baz']) }) # Concatenate two arrays
q = r.table('foo.bar').get(1).update({ 'bar': r.concat(r['bar'], [1, 2, 3]) }) # Concatenate two arrays

q = r.table('foo.bar').map(lambda x: r.slice(x.accomplishments, 0, 3)) # Get first three elements

q = r.table('foo.bar').map(lambda x: r.nth(x.accomplishments, 3)) # Get third element
q = r.table('foo.bar').map(r.nth(r['accomplishments'], 3)) # Get third element
q = r.table('foo.bar').map(r['accomplishments'].nth(3))    # Get third element

q = r.table('foo.posts').map(r.length(r['comments'])) # length of the array
q = r.table('foo.posts').map(r['comments'].length()) # length of the array

# Get all rows where 3rd attribute of an array is 'Alex'
q = r.table('foo.posts').filter(r.eq(r['names'].nth(3), 'Alex'))
q = r.table('foo.posts').filter(r['names'].contains('Alex'))

# Arrays and streams :)
{
  user_id: ...,
  posts: [{ title: ..., text: ... },
          { title: ..., text: ... }
          { title: ..., text: ... }
          { title: ..., text: ... }]
}

q = r.table('foo.users').get(5)              # => ScalarView
                        .attr('posts')       # => Array of { title: ..., text: ... }
                        .filter(True)        # => Stream of { title: ..., text: ... }
                        
q = r.table('foo.users').range(5, 10)        # => RangeView
                        .map(r['posts'])     # => Stream of arrays of { title: ..., text: ... }
                        .filter(True)        # => Stream of arrays of { title: ..., text: ... }

q = r.table('foo.users').range(5, 10)        # => RangeView
                        .map(r['posts'])     # => Stream of arrays of { title: ..., text: ... }
                        .filter(lambda x: )  # => Stream of arrays of { title: ..., text: ... }

users: { user_id, name }
posts {user_id, post_id, title, text}
=>
users: { user_id, name, posts: [{user_id, post_id, title, text}, ...] }

def get_user_posts(user_id):
    return r.db('posts').filter({ 'user_id': user_id})

r.db('users').update({ 'posts': get_user_posts(r['user_id']).to_array() } )

# Concat Map
q = r.db('users').range(10, 20)
                 .concat_map(lambda user: r.table('posts')
                                           .filter(lambda post: post.user_id == user.id)
                                           .map(lambda post: r.extend(user, post)))

# Join
r.join(r.table('users').range(10, 20),
       r.table('posts'),
       lambda user, post: user.id == post.user_id,
       join_type=r.LEFT|r.RIGHT
       left_mapping=...,
       right_mapping=...)

# Let
q = r.table('foo').map(... something expensive ...)
r.table('baz').insert(q)
r.table('bak').insert(q)

q = r.let({ 'query_result': r.table('foo').map(... something expensive ...) },
          [r.table('baz').insert(r.var('query_result')),
           r.table('bak').insert(r.var('query_result'))])

# More complicated
users = { user_id, user_name, age }
posts = { blog_id, user_id, text }


def get_posts_count(user):
    return r.table('posts')
            .filter(r['user_id'] == user.user_id)
            .count()

q = r.table('users').filter(lambda user: user.age == get_posts_count(user))

---

def get_posts_count(user):
    return r.table('posts')
            .filter(r.eq(r['user_id'], user.user_id))
            .count()

q = r.table('users').filter(lambda user: r.eq(user.age, get_posts_count(user)))

---

def get_posts_count(user):
    return r.table('posts')
            .filter(r['user_id'].equals(user.user_id))
            .count()

q = r.table('users').filter(lambda user: user.age.equals(get_posts_count(user)))

---

def get_posts_count(user_id):
    return r.table('posts')
            .filter({ 'user_id': user_id })
            .count()

q = r.table('users').filter({ r['age']: get_posts_count(r['user_id']) })

---

def get_posts_count(user_id):
    return r.table('posts')
            .filter({ 'user_id': user_id })
            .count()

q = r.table('users').filter({ r['age']: get_posts_count(r['user_id']) })

---
def get_posts_count(user):
    return r.table('posts')
            .filter(lambda post: { post.user_id: user.user_id })
            .count()

q = r.table('users').filter( r.eq(r['age'], get_posts_count(r._self)) })

---
def get_posts_count(user):
    return r.table('posts')
            .filter(lambda post: { post.user_id: table.filter(lambda t: iofrijoef) })
            .count()

q = r.table('users').filter(lambda user: r.eq(user.age, get_posts_count(user)) )

q = r.table('users').filter(lambda ctx, user: r.eq(user.age, get_posts_count(user)))
q = r.table('users').filter(lambda user: user.age.eq(get_posts_count(user)))

---

def get_posts_count(user):
    return r.table('posts')
            .filter(lambda post: post.user_id == user.age })
            .count()

q = r.table('users').filter(lambda user: user.age == get_posts_count(user) )

---

# Lazyness/strictness
q = r.table('users').map(fn).limit(10) # fn gets evaluated only ten times
q = r.table('users').orderby('bar').limit(10) # we sort the entire table (ignoring indices)

q = r.table('users').filter(...).map(fn) # fn gets executed only on the result set
q = r.table('users').map(fn).filter(...) # this is strict

# Timing parts of the query, getting execution info (strict/lazy/etc.) -- explain
conn.run(q, error_on_strict=True) # flags only make sense if we don't have a good explain
# Do it on a table/connection level, override on run?

# Error handling...

# For write errors - we complete all the writes that we can, and
# report all errors (possibly truncated + count, if we have time let
# people stream errors).

# For read errors - the moment an error happens, we report it and
# abort if the operation no longer makes sense (i.e. reduction,
# sort). In case of per-row operations, we do the same thing as we do
# for writes (i.e. return what makes sense and report errors).

# Determinism/parallelism/atomicity
q = r.table('foo').update({ 'foo': r['foo'] + r.table('bar').get(r['foo']) }) # Explain
q = r.table('foo').update( { 'foo': r['foo'].incr(), 'bar': r['bar'].incr()}) # Atomic
q = r.table('foo').update( { 'foo': r['bar'].incr(), 'bar': r['foo'].incr()}) # Atomic

# Note: consider
# a.equals(b.add(3))
# r.equals(a, r.add(b, 3))

# r.append(r['bar'], 42)
# vs.
# r['bar'].append(42)   -- this is better (I think)

# lambda x: r.slice(x.accomplishments, 0, 3)
# How do we access attrs of x?

# array.nth(5) vs. r.nth(array, 5)

# Update an element inside an array
                                     
# Notes: we need to pretty print queries (i.e. print q).
# AST return types need to be user friendly - people want to examine
# them.

# Notes: Michel thinks we should break up run into eval and run, where
# run whitelists a set of commands that actually touch tables.

# Alex suggests attr should have a default value (i.e. if attribute is
# missing, use a default).

# Everyone things find may return multiple elements, should be
# find_one or get. Michael thinks find is fine as long as it ALWAYS
# return a scalar.

# Alex things filter should be query.

# Notes find v. filter - people get that.

# Michel thinks range should be named between

# For prefix notation (math), use Clojure semantics

# Should we allow -bar to sort in descending way? Me + Alex think ok,
# Mike, Michel thinks bad. NEWSFLASH: Michael is no longer opposed to
# this.

# Query runtime semantics/explain

# How long it took to run each part of the query

# conn.run(q, optimize=False)  # Use operational semantics

# People prefer sort and orderby to be different functions

# Improve the name for sort_map

# Perhaps inserts should support varargs

# Once connection is worked out, consider r.table('foo')[5] = { ... }
# -- NEWS UPDATE: Michael said FUCK IT!

# Primary key generation

# Make it clear that roundtrips don't happen

# Can we do .update on a find??

# Update returns the count of elements that were updates. It does not
# throw if things are missing.

# Come back to table('foo').all()

# In case of foreach, can we also pass an index of a row?

# Lambdas may be confusing, perhaps row='x'

# Ability to print/debug stuff

# Regex

# Add folds as well

# CoffeeScript and pretty pretty things

# Javascript injection attacks!!

# Rename range to keyrange (or smtg)

# Define difference and intersection ops, skip

# Porcelain for groupby

# How to get attributes from scalar views

# Map vs. each -- why do I have to know the difference?

# To access an entire row (e.g. to do an extend in map, do we require
# them to say 'lambda x: ...')

# Opposite of pluck, iterating over attributes

# Array.nth -- what if nth element doesn't exist.

# q = r.table('foo.posts').map(r.length(r['comments'])) # length of the array
# vs.
# q = r.table('foo.posts').map(r['comments'].length()) # length of the array

# Mike: if expected to mutate -- global function, if not -- member

# Arrays as sets (insert_no_dup, is_member/contains)

# Pickattrs -> pluck on an object

# hasattr (inner attrs?), extend

# injecting arrays in the middle of arrays?

# r['user']['age']
# vs.
# r['user.age']

# How do we deal with primary keys?

# Shorthand syntax for accessing attributes of a single row

# Pluck for tuples, pluck for single attribute?

# Nth element on a stream

# Does each allow you to write to the original row?
# Do we do each on the array?

# r.db('users').update(lambda user: user.posts.set_attr()) <= setting attrs

# Go through every row and remove and attribute -- attribute on
# update, or different function.

# Array/stream ops are polymorphic, explicit conversion during sets

# Upsert should be replace

# r.this to refer to a row?

# If stack depth > 1, always assume ambiguity

# Row.parent
# Get max value row, min value row

# Starting a query off a connection

# Consider insert_into

# GroupedMapReduce

# Key expiration

# Arrays: insert at nth space, beginning, end, remove nth element

# Lets with multiple write queries

# Creating tables/databases, admin crap.

# Packing queries, asynchronisity of queries?

# We actually don't want index/strictness flag, we want explain

# Restricting queries is an environment level control and is a
# separate feature. Possibly a permissions feature.

# Selection/filtering for existence of an attribute.

# Abort queries that run out of ram.

# Dry run

# Geography limitation

# Client has to send the query to the closest possible machine.

# Create tables/databases ad hoc if possible, if not, no problemo :)

# Create/drop/list tables/databases

# Support without along with pluck/pick

# Use table names to disambiguate instead of lambdas

# Primary key types (i.e. sorting ints is different from sorting strings)
