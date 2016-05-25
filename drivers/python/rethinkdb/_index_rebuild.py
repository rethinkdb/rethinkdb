#!/usr/bin/env python

"""'rethinkdb index-rebuild' recreates outdated secondary indexes in a cluster.
  This should be used after upgrading to a newer version of rethinkdb.  There
  will be a notification in the web UI if any secondary indexes are out-of-date."""

from __future__ import print_function

import os, random, sys, time, traceback
from . import utils_common, net
r = utils_common.r

usage = "rethinkdb index-rebuild [-c HOST:PORT] [-n NUM] [-r (DB | DB.TABLE)] [--tls-cert FILENAME] [-p] [--password-file FILENAME]..."
help_epilog = '''
FILE: the archive file to restore data from

EXAMPLES:
rethinkdb index-rebuild -c mnemosyne:39500
  rebuild all outdated secondary indexes from the cluster through the host 'mnemosyne',
  one at a time

rethinkdb index-rebuild -r test -r production.users -n 5
  rebuild all outdated secondary indexes from a local cluster on all tables in the
  'test' database as well as the 'production.users' table, five at a time
'''

# Prefix used for indexes that are being rebuilt
temp_index_prefix = '$reql_temp_index$_'

def parse_options(argv, prog=None):
    parser = utils_common.CommonOptionsParser(usage=usage, epilog=help_epilog, prog=prog)

    parser.add_option("-r", "--rebuild", dest="db_table",   metavar="DB|DB.TABLE", default=[],    help="databases or tables to rebuild indexes on (default: all, may be specified multiple times)",  action="append", type="db_table")
    parser.add_option("-n",              dest="concurrent", metavar="NUM",         default=1,     help="concurrent indexes to rebuild (default: 1)", type="pos_int")
    parser.add_option("--force",         dest="force",      action="store_true",   default=False, help="rebuild non-outdated indexes")

    options, args = parser.parse_args(argv)

    # Check validity of arguments
    if len(args) != 0:
        parser.error("Error: No positional arguments supported. Unrecognized option '%s'" % args[0])

    return options

def rebuild_indexes(options):
    
    # flesh out options.db_table
    if not options.db_table:
        options.db_table = [
            utils_common.DbTable(x['db'], x['name']) for x in
                utils_common.retryQuery('all tables', r.db('rethinkdb').table('table_config').pluck(['db', 'name']))
        ]
    else:
        for db_table in options.db_table[:]: # work from a copy
            if not db_table[1]:
                options.db_table += [utils_common.DbTable(db_table[0], x) for x in utils_common.retryQuery('table list of %s' % db_table[0], r.db(db_table[0]).table_list())]
                del options.db_table[db_table]
    
    # wipe out any indexes with the temp_index_prefix
    for db, table in options.db_table:
        for index in utils_common.retryQuery('list indexes on %s.%s' % (db, table), r.db(db).table(table).index_list()):
            if index.startswith(temp_index_prefix):
                utils_common.retryQuery('drop index: %s.%s:%s' % (db, table, index), r.db(index['db']).table(index['table']).index_drop(index['name']))
    
    # get the list of indexes to rebuild
    indexes_to_build = []
    for db, table in options.db_table:
        indexes = None
        if not options.force:
            indexes = utils_common.retryQuery('get outdated indexes from %s.%s' % (db, table), r.db(db).table(table).index_status().filter({'outdated':True}).get_field('index'))
        else:
            indexes = utils_common.retryQuery('get all indexes from %s.%s' % (db, table), r.db(db).table(table).index_status().get_field('index'))
        for index in indexes:
            indexes_to_build.append({'db':db, 'table':table, 'name':index})
    
    # rebuild selected indexes
    
    total_indexes = len(indexes_to_build)
    indexes_completed = 0
    progress_ratio = 0.0
    highest_progress = 0.0
    indexes_in_progress = []
    
    print("Rebuilding %d index%s: %s" % (total_indexes, 'es' if total_indexes > 1 else '',  ", ".join(["`%(db)s.%(table)s:%(name)s`" % i for i in indexes_to_build])))

    while len(indexes_to_build) > 0 or len(indexes_in_progress) > 0:
        # Make sure we're running the right number of concurrent index rebuilds
        while len(indexes_to_build) > 0 and len(indexes_in_progress) < options.concurrent:
            index = indexes_to_build.pop()
            indexes_in_progress.append(index)
            index['temp_name'] = temp_index_prefix + index['name']
            index['progress'] = 0
            index['ready'] = False

            existingIndexes = dict(
                (x['index'], x['function']) for x in
                utils_common.retryQuery('existing indexes', r.db(index['db']).table(index['table']).index_status().pluck('index', 'function'))
            )
            assert index['name'] in existingIndexes
            if index['temp_name'] not in existingIndexes:
                utils_common.retryQuery(
                    'create temp index: %(db)s.%(table)s:%(name)s' % index,
                    r.db(index['db']).table(index['table']).index_create(index['temp_name'], existingIndexes[index['name']])
                )
        
        # Report progress
        highest_progress = max(highest_progress, progress_ratio)
        utils_common.print_progress(highest_progress)

        # Check the status of indexes in progress
        progress_ratio = 0.0
        for index in indexes_in_progress:
            status = utils_common.retryQuery(
                "progress `%(db)s.%(table)s` index `%(name)s`" % index,
                r.db(index['db']).table(index['table']).index_status(index['temp_name']).nth(0)
            )
            if status['ready']:
                index['ready'] = True
                utils_common.retryQuery(
                    "rename `%(db)s.%(table)s` index `%(name)s`" % index,
                    r.db(index['db']).table(index['table']).index_rename(index['temp_name'], index['name'], overwrite=True)
                )
            else:
                progress_ratio += status.get('progress', 0) / total_indexes

        indexes_in_progress = [index for index in indexes_in_progress if not index['ready']]
        indexes_completed = total_indexes - len(indexes_to_build) - len(indexes_in_progress)
        progress_ratio += float(indexes_completed) / total_indexes

        if len(indexes_in_progress) == options.concurrent or \
           (len(indexes_in_progress) > 0 and len(indexes_to_build) == 0):
            # Short sleep to keep from killing the CPU
            time.sleep(0.1)

    # Make sure the progress bar says we're done and get past the progress bar line
    utils_common.print_progress(1.0)
    print("")

def main(argv=None, prog=None):
    options = parse_options(argv or sys.argv[2:], prog=prog)
    start_time = time.time()
    try:
        rebuild_indexes(options)
    except Exception as ex:
        if options.debug:
            traceback.print_exc()
        print(ex, file=sys.stderr)
        return 1
    print("Done (%d seconds)" % (time.time() - start_time))
    return 0

if __name__ == "__main__":
    sys.exit(main())
