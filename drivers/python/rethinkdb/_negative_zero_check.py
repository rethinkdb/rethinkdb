#!/usr/bin/env python
from __future__ import print_function

import json, math, numbers, os, socket, time
import rethinkdb as r
from optparse import OptionParser
from ._backup import *

info = "'_negative_zero_check` finds and lists inaccessible rows with negative zero in their ID"
usage = "  _negative_zero_check [-c HOST:PORT] [-a AUTH_KEY] [-d DIR]"

def print_negative_zero_check_help():
    print(info)
    print(usage)
    print("")
    print("  -h [ --help ]                    print this help")
    print("  -c [ --connect ] HOST:PORT       host and client port of a rethinkdb node to connect")
    print("                                   to (defaults to localhost:28015)")
    print("  -a [ --auth ] AUTH_KEY           authorization key for rethinkdb clients")
    print("  -f [ --file ] FILE               file to write rows to (stdout by default)")
    print("")
    print("EXAMPLES:")
    print("_negative_zero_check -c mnemosyne:39500")
    print("  List all matching rows from a cluster running on host 'mnemosyne' with a client port at 39500.")
    print("")
    print("_negative_zero_check -d negative_zero_matches")
    print("  Export all matching rows on a local cluster into a named directory.")
    print("")
    print("_negative_zero_check -c hades -a hunter2")
    print("  List all matching rows from a cluster running on host 'hades' which requires authorization.")

def parse_options():
    parser = OptionParser(add_help_option=False, usage=usage)
    parser.add_option("-c", "--connect", dest="host", metavar="HOST:PORT", default="localhost:28015", type="string")
    parser.add_option("-a", "--auth", dest="auth_key", metavar="AUTH_KEY", default="", type="string")
    parser.add_option("-f", "--file", dest="out_file", metavar="FILE", default=None, type="string")
    parser.add_option("-h", "--help", dest="help", default=False, action="store_true")
    (options, args) = parser.parse_args()

    # Check validity of arguments
    if len(args) != 0:
        raise RuntimeError("Error: No positional arguments supported. Unrecognized option '%s'" % args[0])

    if options.help:
        print_negative_zero_check_help()
        exit(0)

    res = {"auth_key": options.auth_key}

    # Verify valid host:port --connect option
    (res["host"], res["port"]) = parse_connect_option(options.host)

    # Verify valid directory option
    if options.out_file is None:
        res["out_file"] = sys.stdout
    else:
        if os.path.exists(options.out_file):
            raise RuntimeError("Error: Output file already exists: %s" % options.out_file)
        res["out_file"] = open(options.out_file, "w+")

    return res

def is_negative_zero(x):
    return x == 0.0 and math.copysign(1.0, x) == -1.0

def key_contains_negative_zero(x):
    if isinstance(x, list):
        return any(map(key_contains_negative_zero, x))
    elif isinstance(x, numbers.Real):
        return is_negative_zero(x)
    return False

# True if the keys are equal, treating negative zero as unique from positive zero
def key_compare(left, right):
    if isinstance(left, list):
        if not isinstance(right, list):
            return False
        return all(map(key_compare, left, right))
    elif isinstance(left, numbers.Real):
        if not isinstance(right, numbers.Real):
            return False
        return left == right and (is_negative_zero(left) == is_negative_zero(right))
    return left == right

def handle_row(db, table, key, is_duplicate, opts, stats):
    stats[(db, table)] += 1
    write_key(opts['out_file'],
              '  ' + json.dumps({ 'db': db,
                                  'table': table,
                                  'key': key,
                                  'is_duplicate': is_duplicate }))

def write_key(out_file, json):
    if not write_key.first_row:
        out_file.write(',\n')
    out_file.write(json)
    write_key.first_row = False

write_key.first_row = True

# Process all rows in the cursor until no more are immediately available
# Returns True if the cursor has more results pending, False if the cursor is completed
def process_cursor(task, c, opts, stats):
    db = task['db']
    table = task['table']
    cursor = task['cursor']
    pkey = task['pkey']
    try:
        while True:
            key = cursor.next(wait=False)
            if key_contains_negative_zero(key):
                # Check if the same row can be found using its key
                row = r.db(db).table(table).get(key) \
                       .run(c, time_format='raw', binary_format='raw')

                if row is None or not key_compare(key, row[pkey]):
                    # We could not retrieve the same row by its key, which means it was
                    # inserted before version 2.0 and is now inaccessible.
                    handle_row(db, table, key, row is not None, opts, stats)
    except socket.timeout as ex:
        return True
    except r.ReqlCursorEmpty as ex:
        return False

def print_summary(opts, stats):
    counts = [count for count in stats.values() if count > 0]
    total = sum(counts)
    if total == 0:
        print("In %d tables, found no rows with negative zero in their primary key." %
              len(stats), file=sys.stderr)
    else:
        print("In %d of %d tables, found %d rows with negative zero in their primary key." %
              (len(counts), len(stats), total), file=sys.stderr)
        for db_table, count in stats.items():
            if count > 0:
                print("  %s: %d" % ('.'.join(db_table), count), file=sys.stderr)

def main():
    try:
        opts = parse_options()
    except Exception as ex:
        print("Usage:\n%s" % usage, file=sys.stderr)
        print(ex, file=sys.stderr)
        return 1

    affected_key_types = r.expr(['NUMBER', 'ARRAY'])
    stats = {}
    tasks = []

    try:
        c = r.connect(opts["host"], opts["port"], auth_key=opts["auth_key"])

        # Make sure the cluster isn't pre-2.0, where positive and negative zero are stored uniquely
        check_minimum_version(None, c, (2, 0, 0))

        for db in r.db_list().set_difference(['rethinkdb']).run(c):
            for table in r.db(db).table_list().run(c):
                stats[(db, table)] = 0
                pkey = r.db(db).table(table).info()['primary_key'].run(c)
                # Only get rows where the primary key is a number or array
                cursor = r.db(db) \
                          .table(table)[pkey] \
                          .filter(lambda x: affected_key_types.contains(x.type_of())) \
                          .run(c, time_format='raw', binary_format='raw')
                tasks.append({'db': db, 'table': table, 'cursor': cursor, 'pkey': pkey})

        opts['out_file'].write('[\n')

        while len(tasks) > 0:
            tasks = [x for x in tasks if process_cursor(x, c, opts, stats)]
            time.sleep(0.1) # Wait a little for more results so we don't kill CPU

        opts['out_file'].write('\n]\n')
        opts['out_file'].flush()

    except Exception as ex:
        print(ex, file=sys.stderr)
        return 1;

    print_summary(opts, stats)

    return 0

if __name__ == "__main__":
    exit(main())
