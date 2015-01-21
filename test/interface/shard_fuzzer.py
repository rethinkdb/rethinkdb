#!/usr/bin/env python
# Copyright 2014 RethinkDB, all rights reserved.

'''This test randomly rebalances tables and shards to probabilistically find bugs in the system.'''

from __future__ import print_function

import pprint, os, sys, time, random, threading

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

opts = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(opts)
opts['random-seed'] = vcoptparse.FloatFlag('--random-seed', random.random())
opts['num-tables'] = vcoptparse.IntFlag('--num-tables', 6) # Number of tables to create
opts['table-scale'] = vcoptparse.IntFlag('--table-scale', 7) # Factor of increasing table size
opts['duration'] = vcoptparse.IntFlag('--duration', 120) # Time to perform fuzzing in seconds
opts['ignore-timeouts'] = vcoptparse.BoolFlag('--ignore-timeouts') # Ignore table_wait timeouts and continue
opts['progress'] = vcoptparse.BoolFlag('--progress') # Write messages every 10 seconds with the time remaining
parsed_opts = opts.parse(sys.argv)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(parsed_opts)

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()

server_names = [ 'War', 'Famine', 'Pestilence', 'Death' ]
tables = [ chr(ord('a') + i) for i in xrange(parsed_opts['num-tables']) ]
table_counts = [ pow(parsed_opts['table-scale'], i) for i in xrange(len(tables)) ]
db = 'test'

# The table_history contains a history of every fuzzed command run against the table
table_history = {}
for table_name in tables:
    table_history[table_name] = []

def populate_table(conn, table, count):
    print("Populating table '%s' with %d rows (%.2fs)" % (table, count, time.time() - startTime))
    r.db(db).table(table).insert(r.range(count).map(lambda x: {'id': x})).run(conn)

def create_tables(conn):
    assert len(tables) == len(table_counts)
    if not dbName in r.db_list().run(conn):
        r.db_create(dbName).run(conn)

    print("Creating %d tables (%.2fs)" % (len(tables), time.time() - startTime))
    for i in xrange(len(tables)):
        r.db(db).table_create(tables[i]).run(conn)
        populate_table(conn, tables[i], table_counts[i])

def generate_fuzz():
    shards = []
    for i in xrange(random.randint(1, 16)):
        shards.append({'replicas': random.sample(server_names, random.randint(1, len(server_names)))})
        shards[-1]['primary_replica'] = random.choice(shards[-1]['replicas'])
    return {'shards': shards}    

def fuzz_table(cluster, table, stop_event, random_seed):
    random.seed(random_seed)
    fuzz_attempts = 0
    fuzz_successes = 0
    fuzz_non_trivial_failures = 0

    # All the query-related functions used in the fuzzer loop
    def do_query(fn, c):
        query = fn(r.db(db).table(table))
        table_history[table].append(query)
        return query.run(c)

    def table_wait(q):
        return q.wait(wait_for='all_replicas_ready', timeout=30)

    def table_reconfigure(q):
        return q.reconfigure(shards=random.randint(1, 16), replicas=random.randint(1, len(server_names)))

    def table_rebalance(q):
        return q.rebalance()

    def table_config_update(q):
        return q.config().update(generate_fuzz())

    while not stop_event.is_set():
        try:
            server = random.choice(list(cluster.processes))
            conn = r.connect(server.host, server.driver_port)
        except:
            print("Failed to connect to a server - something probably broke, stopping fuzz")
            stop_event.set()
            continue

        try:
            fuzz_attempts += 1
            if random.random() > 0.2: # With an 80% probability, wait for the table before fuzzing
                do_query(table_wait, conn)

            rand_res = random.random()
            if rand_res > 0.9:   # With a 10% probability, do a basic `reconfigure`
                do_query(table_reconfigure, conn)
            elif rand_res > 0.8: # With a 10% probability, use `rebalance`
                do_query(table_rebalance, conn)
            else:                # With an 80% probability, use a fuzzed configuration
                do_query(table_config_update, conn)
            fuzz_successes += 1
        except Exception as ex:
            # Ignore some errors that are natural consequences of the test
            if "isn't enough data in the table" in str(ex) or \
               "the table isn't currently available for reading" in str(ex) or \
               (parsed_opts['ignore-timeouts'] and "Timed out while waiting for tables" in str(ex)):
               pass
            else:
                fuzz_non_trivial_failures += 1
                print("Fuzz of table '%s' failed (%.2fs): %s" % (table, time.time() - startTime, str(ex)))
                try:
                    (config, status) = r.expr([r.db(db).table(table).config(), r.db(db).table(table).status()]).run(conn)
                    print("Table '%s' config:\n%s" % (table, pprint.pformat(config)))
                    print("Table '%s' status:\n%s" % (table, pprint.pformat(status)))
                except Exception as ex:
                    print("Could not get config or status for table '%s': %s" % (table, str(ex)))
                print("Table '%s' history:\n%s" % (table, pprint.pformat(table_history[table])))
    print("Stopped fuzzing on table '%s', attempts: %d, successes: %d, non-trivial failures: %d (%.2fs)" %
          (table, fuzz_attempts, fuzz_successes, fuzz_non_trivial_failures, time.time() - startTime))
    sys.stdout.flush()

print("Spinning up %d servers (%.2fs)" % (len(server_names), time.time() - startTime))
with driver.Cluster(initial_servers=server_names, output_folder='.', command_prefix=command_prefix,
                    extra_options=serve_options, wait_until_ready=True) as cluster:
    cluster.check()
    print("table counts: %s" % table_counts)
    
    print("Server driver ports: %s" % (str([x.driver_port for x in cluster])))
    print("Establishing ReQL connection (%.2fs)" % (time.time() - startTime))
    conn = r.connect(host=cluster[0].host, port=cluster[0].driver_port)
    create_tables(conn)

    random.seed(parsed_opts['random-seed'])
    print("Fuzzing shards for %ds, random seed: %f (%.2fs)" %
          (parsed_opts['duration'], parsed_opts['random-seed'], time.time() - startTime))
    stop_event = threading.Event()
    table_threads = []
    for table in tables:
        table_threads.append(threading.Thread(target=fuzz_table, args=(cluster, table, stop_event, random.random())))
        table_threads[-1].start()

    last_time = time.time()
    end_time = last_time + parsed_opts['duration']
    while (time.time() < end_time) and not stop_event.is_set():
        # TODO: random disconnections / kills during fuzzing
        time.sleep(0.2)
        current_time = time.time()
        if parsed_opts['progress'] and int((end_time - current_time) / 10) < int((end_time - last_time) / 10):
            print("%ds remaining (%.2fs)" % (int(end_time - current_time) + 1, time.time() - startTime))
        last_time = current_time
        if not all([x.is_alive() for x in table_threads]):
            stop_event.set()

    print("Stopping fuzzing (%d of %d threads remain) (%.2fs)" % (len(table_threads), len(tables), time.time() - startTime))
    stop_event.set()
    for thread in table_threads:
        thread.join()

    for i in xrange(len(tables)):
        print("Checking contents of table '%s' (%.2fs)" % (tables[i], time.time() - startTime))
        # TODO: check row data itself
        r.db(db).table(tables[i]).wait().run(conn)
        count = r.db(db).table(tables[i]).count().run(conn)
        assert count == table_counts[i], "Incorrect table count following fuzz of table '%s', found %d of expected %d" % (tables[i], count, table_counts[i])
        
    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))

