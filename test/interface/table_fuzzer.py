#!/usr/bin/env python
# Copyright 2014 RethinkDB, all rights reserved.

'''This test randomly runs db, table, and index creates/drops, as well as reads, writes,
and optionally changefeeds across any number of client threads and any number of servers
in a cluster.'''

import bisect, os, random, resource, string, sys, time, threading

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

try:
    xrange
except NameError:
    xrange = range

# Warn if this is run without unlimited size core files - which are really useful for debugging
if resource.getrlimit(resource.RLIMIT_CORE)[0] != resource.RLIM_INFINITY:
    utils.print_with_time("Warning, running without unlimited size core files")

opts = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(opts)
opts['random-seed'] = vcoptparse.FloatFlag('--random-seed', random.random())
opts['servers'] = vcoptparse.IntFlag('--servers', 1) # Number of servers in the cluster
opts['duration'] = vcoptparse.IntFlag('--duration', 900) # Time to perform fuzzing in seconds
opts['progress'] = vcoptparse.BoolFlag('--progress', False) # Write messages every 10 seconds with the time remaining
opts['threads'] = vcoptparse.IntFlag('--threads', 16) # Number of client threads to run (not counting changefeeds)
opts['changefeeds'] = vcoptparse.BoolFlag('--changefeeds', False) # Whether or not to use changefeeds
opts['kill'] = vcoptparse.BoolFlag('--kill', False) # Randomly kill and revive servers during fuzzing - will produce a lot of noise
parsed_opts = opts.parse(sys.argv)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(parsed_opts)

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()

server_names = list(string.ascii_lowercase[:parsed_opts['servers']])

system_tables = ['table_config',
                 'server_config',
                 'db_config',
                 'cluster_config',
                 'table_status',
                 'server_status',
                 'current_issues',
                 'jobs',
                 'stats',
                 'logs',
                 '_debug_table_status']

# Global data used by query generators, and a lock to make it thread-safe
data_lock = threading.Lock()
dbs = set()
tables = set()
indexes = set()

#   Db, Table, Index
# ---------------------------------------------------------------------------------------
# These classes are used to track which operations are currently legal.
#
# Each of them exists in the globally-accessible sets `dbs`, `tables`, and `indexes`, and will
# reference their parent/children objects.  The `unlink` function is used once we no longer
# need to remember the object (typically, after receiving a reply from the server when we
# delete it).  The lifetime of these objects is specifially designed such that we can write
# queries using them before they are constructed or after they are deleted and thus test for
# racy behaviors in the server.
#
# Note that if we ever introduce tests that put the lifetime of these objects in an
# indeterminate state, we may need a periodic 'resync' of the global tables to the current
# state of the objects in the cluster.  This would necessarily lock out all queries while
# going on.
class Db:
    def __init__(self, name):
        self.name = name
        self.tables = set()
        dbs.add(self)
    def unlink(self):
        for table in list(self.tables):
            table.unlink()
        if self in dbs:
            dbs.remove(self)

class Table:
    def __init__(self, db, name):
        self.db = db
        self.name = name
        self.indexes = set()
        self.count = 0
        db.tables.add(self)
        tables.add(self)
        self.indexes.add(Index(self, 'id'))
    def unlink(self):
        for index in list(self.indexes):
            index.unlink()
        if self in self.db.tables:
            self.db.tables.remove(self)
        if self in tables:
            tables.remove(self)

class Index:
    def __init__(self, table, name):
        self.table = table
        self.name = name
        table.indexes.add(self)
        indexes.add(self)
    def unlink(self):
        if self in self.table.indexes:
            self.table.indexes.remove(self)
        if self in indexes:
            indexes.remove(self)

# Utility functions for generating queries
def make_name():
    return ''.join(random.choice(string.ascii_lowercase) for i in xrange(4))

def rand_shards():
    return random.randint(1, 16)

def rand_replicas():
    return random.randint(1, len(server_names))

def rand_db():
    return random.sample(dbs, 1)[0]

def rand_table():
    return random.sample(tables, 1)[0]

def rand_index():
    return random.sample(indexes, 1)[0]

def rand_bool():
    return random.randint(0, 1) == 1

def accumulate(iterable):
    it = iter(iterable)
    total = next(it)
    yield total
    for element in it:
        total = total + element
        yield total

# Performs a random choice of an operation to perform.  Input is an array of tuples:
# (op_type, weight), where weight is a number.  The randomization is normalized based
# on the sum of all weights in the set.
def weighted_random(weighted_ops):
    ops, weights = zip(*weighted_ops)
    distribution = list(accumulate(weights))
    chosen_weight = random.random() * distribution[-1]
    return ops[bisect.bisect(distribution, chosen_weight)]

def check_error(context, reql_error):
    if parsed_opts['kill'] and reql_error.message == 'Connection is closed.':
        pass
    else:
        utils.print_with_time('\n%s resulted in Exception: %s' % (str(context), repr(reql_error)))

# Performs a single operation chosen from the operations in `weighted_ops` which report
# themselves to be valid based on the current state of `dbs`, `tables`, and `indexes`.
# These global sets may be changed each before and after the operation is performed.
def run_random_query(conn, weighted_ops):
    try:
        data_lock.acquire()
        try:
            weighted_ops = [x for x in weighted_ops if x[0].is_valid()]
            op_type = weighted_random(weighted_ops)
            op = op_type(conn)
        finally:
            data_lock.release()

        q = op.get_query()
        q.run(conn)
        sys.stdout.write('.')
        sys.stdout.flush()

        data_lock.acquire()
        try:
            op.post_run()
        finally:
            data_lock.release()
    except r.ReqlAvailabilityError as ex:
        pass # These are perfectly normal during fuzzing due to concurrent queries
    except r.ReqlError as ex:
        check_error("Query " + str(q), ex)

# Query, DbQuery, TableQuery, IndexQuery
# ---------------------------------------------------------------------------------------
# Base classes for operation types - organized by what objects are required for the
# operation to be valid.  A `Query` is always valid.  A `DbQuery` requires at least one
# `Db` to exist.  A `TableQuery` requires at least one `Table` to exist.  An `IndexQuery`
# requires at least one `Index` to exist.
#
# For succinctness, these may pass a pre-generated query stub to a child class's
# `sub_query` method, which they can chain their query off of. A child class may instead
# choose to overload `get_query` instead if it proves useful.
class Query:
    @staticmethod
    def is_valid():
        return True
    def __init__(self, conn):
        self.conn = conn
    def get_query(self):
        return self.sub_query(r)
    def post_run(self):
        pass

class DbQuery:
    @staticmethod
    def is_valid():
        return len(dbs) > 0
    def __init__(self, conn):
        self.conn = conn
        self.db = rand_db()
    def get_query(self):
        return self.sub_query(r.db(self.db.name))
    def post_run(self):
        pass

class TableQuery:
    @staticmethod
    def is_valid():
        return len(tables) > 0
    def __init__(self, conn):
        self.conn = conn
        self.table = rand_table()
    def get_query(self):
        return self.sub_query(r.db(self.table.db.name).table(self.table.name))
    def post_run(self):
        pass

class IndexQuery:
    @staticmethod
    def is_valid():
        return len(indexes) > 0
    def __init__(self, conn):
        self.conn = conn
        self.index = rand_index()
    def get_query(self):
        return self.sub_query(r.db(self.index.table.db.name).table(self.index.table.name))
    def post_run(self):
        pass

# No requirements
class db_create(Query):
    def __init__(self, conn):
        Query.__init__(self, conn)
        self.db = Db(make_name())
    def sub_query(self, q):
        if rand_bool():
            return r.db_create(self.db.name)
        else:
            return r.db('rethinkdb').table('db_config') \
                    .insert({'name': self.db.name})

class system_table_read(Query):
    def sub_query(self, q):
        table = random.choice(system_tables)
        return r.db('rethinkdb').table(table).coerce_to('array')

# Requires a DB
class db_drop(DbQuery):
    def sub_query(self, q):
        if rand_bool():
            return r.db_drop(self.db.name)
        else:
            return r.db('rethinkdb').table('db_config') \
                    .filter(r.row['name'].eq(self.db.name)).delete()
    def post_run(self):
        self.db.unlink()

class table_create(DbQuery):
    def __init__(self, conn):
        DbQuery.__init__(self, conn)
        self.table = Table(self.db, make_name())
    def sub_query(self, q):
        if rand_bool():
            return q.table_create(self.table.name)
        else:
            return r.db('rethinkdb').table('table_config') \
                    .insert({'name': self.table.name, 'db': self.db.name})

# Requires a Table
class wait(TableQuery):
    def sub_query(self, q):
        wait_for = random.choice(['all_replicas_ready',
                                  'ready_for_writes',
                                  'ready_for_reads',
                                  'ready_for_outdated_reads'])
        return q.wait(wait_for=wait_for, timeout=30)

class reconfigure(TableQuery):
    def sub_query(self, q):
        return q.reconfigure(shards=rand_shards(), replicas=rand_replicas())

class rebalance(TableQuery):
    def sub_query(self, q):
        return q.rebalance()

class config_update(TableQuery):
    def sub_query(self, q):
        shards = []
        for i in xrange(rand_shards()):
            shards.append({'replicas': random.sample(server_names, rand_replicas())})
            shards[-1]['primary_replica'] = random.choice(shards[-1]['replicas'])
        return q.config().update({'shards': shards})

class insert(TableQuery):
    def __init__(self, conn):
        TableQuery.__init__(self, conn)
        self.start = self.table.count
        self.num_rows = random.randint(1, 500)
        self.table.count = self.start + self.num_rows
    def sub_query(self, q):
        return q.insert(r.range(self.start, self.start + self.num_rows).map(lambda x: {'id': x}))

class table_drop(TableQuery):
    def sub_query(self, q):
        if rand_bool():
            return q.config().delete()
        else:
            return r.db('rethinkdb').table('table_config') \
                    .filter(r.row['name'].eq(self.table.name)).delete()
    def post_run(self):
        self.table.unlink()

class index_create(TableQuery):
    def __init__(self, conn):
        TableQuery.__init__(self, conn)
        self.index = Index(self.table, make_name())
    def sub_query(self, q):
        return q.index_create(self.index.name)

# Requires an Index
class index_drop(IndexQuery):
    def sub_query(self, q):
        return q.index_drop(self.index.name)
    def post_run(self):
        self.index.unlink()

# Changefeeds
def run_changefeed(query, host, port):
    def thread_fn(self, host, port):
        duration = 10
        end_time = time.time() + duration
        try:
            conn = r.connect(host, port)
            feed = query.changes().run(conn)
            while time.time() < end_time:
                try:
                    feed.next(wait=duration)
                except r.ReqlTimeoutError:
                    pass
        except r.ReqlAvailabilityError as ex:
            pass # These are perfectly normal during fuzzing due to concurrent queries
        except r.ReqlError as ex:
            check_error("Feed", ex)
    feed_thread = threading.Thread(target=thread_fn, args=(query, host, port))
    feed_thread.daemon = True
    feed_thread.start()

class changefeed(IndexQuery):
    def sub_query(self, q):
        run_changefeed(r.db(self.index.table.db.name) \
                        .table(self.index.table.name) \
                        .between(r.minval, r.maxval, index=self.index.name),
                       self.conn.host, self.conn.port)
        return r.expr(0) # dummy query for silly reasons

class system_changefeed(Query):
    def sub_query(self, q):
        run_changefeed(r.db('rethinkdb').table(random.choice(system_tables)),
                       self.conn.host, self.conn.port)
        return r.expr(0) # dummy query for silly reasons

def do_fuzz(cluster, cluster_lock, stop_event, random_seed):
    random.seed(random_seed)
    weighted_ops = [(db_create, 4),
                    # (db_rename, 3), # Renames are racy and so currently omitted
                    (db_drop, 3),
                    (table_create, 8),
                    # (table_rename, 6), # Renames are racy and so currently omitted
                    (table_drop, 6),
                    (index_create, 8),
                    # (index_rename, 4), # Renames are racy and so currently omitted
                    (index_drop, 2),
                    (insert, 100),
                    (rebalance, 10),
                    (reconfigure, 10),
                    (config_update, 10),
                    (wait, 10),
                    (system_table_read, 10)]

    if parsed_opts['changefeeds']:
        weighted_ops.append((changefeed, 10))
        weighted_ops.append((system_changefeed, 10))

    while not stop_event.is_set():
        server = None
        with cluster_lock:
            try:
                server = random.choice([p for p in cluster if p.ready])
            except IndexError:
                pass

        if server is not None:
            try:
                conn = r.connect(server.host, server.driver_port)
            except r.ReqlError as ex:
                check_error("Connection creation", ex)

            while conn.is_open() and not stop_event.is_set():
                run_random_query(conn, weighted_ops)
        else:
            time.sleep(1) # No ready servers - try again later

def kill_random_servers(cluster):
    alive_procs = [p for p in cluster if p.running]
    chosen_procs = random.sample(alive_procs, random.randint(1, len(alive_procs)))
    remaining = len(alive_procs) - len(chosen_procs) + 1
    utils.print_with_time("\nKilling %d servers, %d remain" % (len(chosen_procs), remaining))
    [p.kill() for p in chosen_procs]

def revive_random_servers(cluster):
    dead_procs = [p for p in cluster if not p.running]
    chosen_procs = random.sample(dead_procs, random.randint(1, len(dead_procs)))
    remaining = len(cluster) - len(dead_procs) + len(chosen_procs) + 1
    utils.print_with_time("\nReviving %d servers, %d remain" % (len(chosen_procs), remaining))
    [p.start(wait_until_ready=False) for p in chosen_procs]

def server_kill_thread(cluster, cluster_lock, stop_event):
    # Every period we kill or revive a random set of servers
    while not stop_event.is_set():
        with cluster_lock:
            usable = cluster[1:]
            if all([p.running for p in usable]):
                kill_random_servers(usable)
            elif all([not p.running for p in usable]):
                revive_random_servers(usable)
            elif random.random() < 0.5:
                kill_random_servers(usable)
            else:
                revive_random_servers(usable)

            if any([p.running for p in usable]) and random.random() < 80:
                time.sleep(random.random() * 10)

utils.print_with_time("Spinning up %d servers" % (len(server_names)))
with driver.Cluster(initial_servers=server_names, output_folder='.', command_prefix=command_prefix,
                    extra_options=serve_options, wait_until_ready=True) as cluster:
    cluster.check()
    random.seed(parsed_opts['random-seed'])

    utils.print_with_time("Server driver ports: %s" % (str([x.driver_port for x in cluster])))
    utils.print_with_time("Fuzzing for %ds, random seed: %s" %
          (parsed_opts['duration'], repr(parsed_opts['random-seed'])))
    stop_event = threading.Event()
    cluster_lock = threading.Lock()
    fuzz_threads = []

    for i in xrange(parsed_opts['threads']):
        fuzz_threads.append(threading.Thread(target=do_fuzz, args=(cluster, cluster_lock, stop_event, random.random())))
        fuzz_threads[-1].start()

    if parsed_opts['kill']:
        fuzz_threads.append(threading.Thread(target=server_kill_thread, args=(cluster, cluster_lock, stop_event)))
        fuzz_threads[-1].start()

    last_time = time.time()
    end_time = last_time + parsed_opts['duration']
    try:
        while (time.time() < end_time) and all([x.is_alive() for x in fuzz_threads]) and not stop_event.is_set():
            time.sleep(0.2)
            current_time = time.time()
            if parsed_opts['progress'] and int((end_time - current_time) / 10) < int((end_time - last_time) / 10):
                utils.print_with_time("\n%ds remaining" % (int(end_time - current_time) + 1))
            last_time = current_time
    finally:
        utils.print_with_time("\nStopping fuzzing (%d of %d threads remain)" %
              (len(fuzz_threads), parsed_opts['threads']))
        stop_event.set()
        for thread in fuzz_threads:
            thread.join()

    utils.print_with_time("\nCleaning up")
utils.print_with_time("Done")

