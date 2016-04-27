#!/usr/bin/env python

from __future__ import print_function

import optparse, os, random, sys, time
from . import utils_common, net
r = utils_common.r

info = """'rethinkdb index-rebuild' recreates outdated secondary indexes in a cluster.
  This should be used after upgrading to a newer version of rethinkdb.  There
  will be a notification in the web UI if any secondary indexes are out-of-date."""
usage = "rethinkdb index-rebuild [-c HOST:PORT] [-n NUM] [-r (DB | DB.TABLE)] [--tls-cert FILENAME] [-p] [--password-file FILENAME]..."

# Prefix used for indexes that are being rebuilt
temp_index_prefix = '$reql_temp_index$_'

def print_restore_help():
    print(info)
    print(usage)
    print("""
  FILE                             the archive file to restore data from
  -h [ --help ]                    print this help
  -c [ --connect ] HOST:PORT       host and client port of a rethinkdb node to connect
                                   to (defaults to localhost:%d)
  --tls-cert FILENAME              certificate file to use for TLS encryption.
  -p [ --password ]                interactively prompt for a password required to connect.
  --password-file FILENAME         read password required to connect from file.
  -r [ --rebuild ] (DB | DB.TABLE) the databases or tables to rebuild indexes on
                                   (defaults to all databases and tables)
  -n NUM                           the number of concurrent indexes to rebuild
                                   (defaults to 1)

EXAMPLES:
rethinkdb index-rebuild -c mnemosyne:39500
  rebuild all outdated secondary indexes from the cluster on the host 'mnemosyne',
  one at a time

rethinkdb index-rebuild -r test -r production.users -n 5
  rebuild all outdated secondary indexes from a local cluster on all tables in the
  'test' database as well as the 'production.users' table, five at a time""" % net.DEFAULT_PORT)

def parse_options(argv):
    parser = optparse.OptionParser(add_help_option=False, usage=usage)
    parser.add_option("-c", "--connect", dest="host", metavar="HOST:PORT")
    parser.add_option("-r", "--rebuild", dest="tables", metavar="DB | DB.TABLE", default=[], action="append")
    parser.add_option("--tls-cert", dest="tls_cert", metavar="TLS_CERT", default="")

    parser.add_option("-n", dest="concurrent", metavar="NUM", default=1, type="int")
    parser.add_option("--debug", dest="debug", default=False, action="store_true")
    parser.add_option("-h", "--help", dest="help", default=False, action="store_true")
    parser.add_option("-p", "--password", dest="password", default=False, action="store_true")
    parser.add_option("--password-file", dest="password_file", default=None)

    (options, args) = parser.parse_args(argv)

    if options.help:
        print_restore_help()
        exit(0)

    # Check validity of arguments
    if len(args) != 0:
        raise RuntimeError("Error: No positional arguments supported")

    res = { }

    # Verify valid host:port --connect option
    (res["host"], res["port"]) = utils_common.parse_connect_option(options.host)

    res["tls_cert"] = utils_common.ssl_option(options.tls_cert)
    # Verify valid --import options
    res["tables"] = utils_common.parse_db_table_options(options.tables)

    res["concurrent"] = options.concurrent
    res["debug"] = options.debug

    res["password"] = utils_common.get_password(options.password, options.password_file)
    return res

def do_connect(options):
    try:
        return r.connect(options['host'],
                         options['port'],
                         ssl = options['tls_cert'],
                         user="admin",
                         password=options["password"])
    except (r.ReqlError, r.ReqlDriverError) as ex:
        raise RuntimeError("Error when connecting: %s" % ex.message)

def new_connection(conn_store, options):
    # Check if the connection is up
    try:
        r.expr(0).run(conn_store[0])
    except:
        conn_store[0] = do_connect(options)
    return conn_store[0]

def get_table_outdated_indexes(conn, db, table):
    return r.db(db).table(table).index_status().filter(lambda i: i['outdated'])['index'].run(conn)

def get_outdated_indexes(progress, conn, db_tables):
    res = [ ]
    if len(db_tables) == 0:
        dbs = r.db_list().run(conn)
        db_tables = [(db, None) for db in dbs]

    for db_table in db_tables:
        if db_table[1] is not None:
            table_list = [db_table[1]]
        else:
            table_list = r.db(db_table[0]).table_list().run(conn)
        for table in table_list:
            outdated_indexes = get_table_outdated_indexes(conn, db_table[0], table)

            for index in outdated_indexes:
                res.append({'db': db_table[0], 'table': table, 'name': index})
    return res

def drop_outdated_temp_indexes(progress, conn, indexes):
    indexes_to_drop = [i for i in indexes if i['name'].find(temp_index_prefix) == 0]
    for index in indexes_to_drop:
        r.db(index['db']).table(index['table']).index_drop(index['name']).run(conn)
        indexes.remove(index)

def create_temp_index(progress, conn, index):
    # If this index is already being rebuilt, don't try to recreate it
    extant_indexes = r.db(index['db']).table(index['table']).index_status().map(lambda i: i['index']).run(conn)
    if index['temp_name'] not in extant_indexes:
        index_fn = r.db(index['db']).table(index['table']).index_status(index['name']).nth(0)['function']
        r.db(index['db']).table(index['table']).index_create(index['temp_name'], index_fn).run(conn)

def get_index_progress(progress, conn, index):
    status = r.db(index['db']).table(index['table']).index_status(index['temp_name']).nth(0).run(conn)
    index['function'] = status['function']
    if status['ready']:
        return None
    else:
        return float(status.get('progress'))

def rename_index(progress, conn, index):
    r.db(index['db']).table(index['table']).index_rename(index['temp_name'], index['name'], overwrite=True).run(conn)

def check_index_renamed(progress, conn, index):
    status = r.db(index['db']).table(index['table']).index_status(index['name']).nth(0).run(conn)
    if status['outdated'] or status['ready'] != True or status['function'] != index['function']:
        raise RuntimeError("Error: failed to rename `%s.%s` temporary index for `%s`" % \
                           (index['db'], index['table'], index['name']))

def rebuild_indexes(options):
    conn_store = [do_connect(options)]
    conn_fn = lambda: new_connection(conn_store, options)

    indexes_to_build = utils_common.rdb_call_wrapper(conn_fn, "get outdated indexes", get_outdated_indexes, options["tables"])
    indexes_in_progress = [ ]

    # Drop any outdated indexes with the temp_index_prefix
    utils_common.rdb_call_wrapper(conn_fn, "drop temporary outdated indexes", drop_outdated_temp_indexes, indexes_to_build)

    random.shuffle(indexes_to_build)
    total_indexes = len(indexes_to_build)
    indexes_completed = 0
    progress_ratio = 0.0
    highest_progress = 0.0

    print("Rebuilding %d indexes: %s" % (total_indexes,
                                         ", ".join(map(lambda i: "`%s.%s:%s`" % (i['db'], i['table'], i['name']),
                                                       indexes_to_build))))

    while len(indexes_to_build) > 0 or len(indexes_in_progress) > 0:
        # Make sure we're running the right number of concurrent index rebuilds
        while len(indexes_to_build) > 0 and len(indexes_in_progress) < options["concurrent"]:
            index = indexes_to_build.pop()
            indexes_in_progress.append(index)
            index['temp_name'] = temp_index_prefix + index['name']
            index['progress'] = 0
            index['ready'] = False

            try:
                utils_common.rdb_call_wrapper(conn_fn, "create `%s.%s` index `%s`" % (index['db'], index['table'], index['name']),
                                              create_temp_index, index)
            except RuntimeError as ex:
                # This may be caused by a suprious failure (see github issue #2904), ignore if so
                if ex.message != "ReQL error during 'create `%s.%s` index `%s`': Index `%s` already exists on table `%s.%s`." % \
                                 (index['db'], index['table'], index['name'], index['temp_name'], index['db'], index['table']):
                    raise

        # Report progress
        highest_progress = max(highest_progress, progress_ratio)
        utils_common.print_progress(highest_progress)

        # Check the status of indexes in progress
        progress_ratio = 0.0
        for index in indexes_in_progress:
            index_progress = utils_common.rdb_call_wrapper(conn_fn, "progress `%s.%s` index `%s`" % (index['db'], index['table'], index['name']),
                                                           get_index_progress, index)
            if index_progress is None:
                index['ready'] = True
                try:
                    utils_common.rdb_call_wrapper(conn_fn, "rename `%s.%s` index `%s`" % (index['db'], index['table'], index['name']),
                                                  rename_index, index)
                except r.ReqlRuntimeError as ex:
                    # This may be caused by a spurious failure (see github issue #2904), check if it actually succeeded
                    if ex.message != "ReQL error during 'rename `%s.%s` index `%s`': Index `%s` does not exist on table `%s.%s`." % \
                                     (index['db'], index['table'], index['name'], index['temp_name'], index['db'], index['table']):
                        raise
                    utils_common.rdb_call_wrapper(conn_fn, "check rename `%s.%s` index `%s`" % (index['db'], index['table'], index['name']),
                                                  check_index_renamed, index)
            else:
                progress_ratio += index_progress / total_indexes

        indexes_in_progress = [index for index in indexes_in_progress if not index['ready']]
        indexes_completed = total_indexes - len(indexes_to_build) - len(indexes_in_progress)
        progress_ratio += float(indexes_completed) / total_indexes

        if len(indexes_in_progress) == options['concurrent'] or \
           (len(indexes_in_progress) > 0 and len(indexes_to_build) == 0):
            # Short sleep to keep from killing the CPU
            time.sleep(0.1)

    # Make sure the progress bar says we're done and get past the progress bar line
    utils_common.print_progress(1.0)
    print("")

def main(argv=None):
    if argv is None:
        argv = sys.argv[1:]
    try:
        options = parse_options(argv)
    except RuntimeError as ex:
        print("Usage: %s" % usage, file=sys.stderr)
        print(ex, file=sys.stderr)
        return 1

    try:
        start_time = time.time()
        rebuild_indexes(options)
    except RuntimeError as ex:
        print(ex, file=sys.stderr)
        return 1
    print("  Done (%d seconds)" % (time.time() - start_time))
    return 0

if __name__ == "__main__":
    exit(main())
