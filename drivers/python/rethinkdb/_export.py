#!/usr/bin/env python

from __future__ import print_function

import collections, csv, ctypes, datetime, json, math, multiprocessing, numbers
import optparse, os, platform, tempfile, re, signal, sys, time, traceback

from . import errors, net, query, utils_common

try:
    unicode
except NameError:
    unicode = str
try:
    from Queue import Empty, Full
except ImportError:
    from queue import Empty, Full
try:
    from multiprocessing import Queue, SimpleQueue
except ImportError:
    from multiprocessing.queues import Queue, SimpleQueue

usage = """rethinkdb export [-c HOST:PORT] [-p] [--password-file FILENAME] [--tls-cert filename] [-d DIR] [-e (DB | DB.TABLE)]...
      [--format (csv | json | ndjson)] [--fields FIELD,FIELD...] [--delimiter CHARACTER]
      [--clients NUM]"""
help_description = '`rethinkdb export` exports data from a RethinkDB cluster into a directory'
help_epilog = '''
EXAMPLES:
rethinkdb export -c mnemosyne:39500
  Export all data from a cluster running on host 'mnemosyne' with a client port at 39500.

rethinkdb export -e test -d rdb_export
  Export only the 'test' database on a local cluster into a named directory.

rethinkdb export -c hades -e test.subscribers -p
  Export a specific table from a cluster running on host 'hades' which requires a password.

rethinkdb export --format csv -e test.history --fields time,message --delimiter ';'
  Export a specific table from a local cluster in CSV format with the fields 'time' and 'message',
  using a semicolon as field delimiter (rather than a comma).

rethinkdb export --fields id,value -e test.data
  Export a specific table from a local cluster in JSON format with only the fields 'id' and 'value'.
'''

def parse_options(argv, prog=None):
    if platform.system() == "Windows" or platform.system().lower().startswith('cygwin'):
        defaultDir = "rethinkdb_export_%s" % datetime.datetime.today().strftime("%Y-%m-%dT%H-%M-%S") # no colons in name
    else:
        defaultDir = "rethinkdb_export_%s" % datetime.datetime.today().strftime("%Y-%m-%dT%H:%M:%S") # "
    
    parser = utils_common.CommonOptionsParser(usage=usage, description=help_description, epilog=help_epilog, prog=prog)
    
    parser.add_option("-d", "--directory", dest="directory", metavar="DIRECTORY",       default=defaultDir, help='directory to output to (default: rethinkdb_export_DATE_TIME)', type="new_file")
    parser.add_option("-e", "--export",    dest="db_tables", metavar="DB|DB.TABLE",     default=[],         help='limit dump to the given database or table (may be specified multiple times)', action="append", type="db_table")
    parser.add_option("--fields",          dest="fields",    metavar="<FIELD>,...",     default=None,       help='export only specified fields (required for CSV format)')
    parser.add_option("--format",          dest="format",    metavar="json|csv|ndjson", default="json",     help='format to write (defaults to json. ndjson is newline delimited json.)', type="choice", choices=['json', 'csv', 'ndjson'])
    parser.add_option("--clients",         dest="clients",   metavar="NUM",             default=3,          help='number of tables to export simultaneously (default: 3)', type="pos_int")
    parser.add_option("--read-outdated",   dest="outdated",                             default=False,      help='use outdated read mode',  action="store_true")
    
    csvGroup = optparse.OptionGroup(parser, 'CSV options')
    csvGroup.add_option("--delimiter", dest="delimiter",     metavar="CHARACTER",       default=None,       help="character to be used as field delimiter, or '\\t' for tab (default: ',')")
    parser.add_option_group(csvGroup)
    
    options, args = parser.parse_args(argv)
    
    # -- Check validity of arguments
    
    if len(args) != 0:
        parser.error("No positional arguments supported. Unrecognized option(s): %s" % args)
    
    if options.fields:
        if len(options.db_tables) != 1 or options.db_tables[0].table is None:
            parser.error("The --fields option can only be used when exporting a single table")
        options.fields = options.fields.split(",")
    
    # - format specific validation
    
    if options.format == "csv":
        if options.fields is None:
            parser.error("CSV files require the '--fields' option to be specified.")
        
        if options.delimiter is None: 
            options.delimiter = ","
        elif options.delimiter == "\\t":
            options.delimiter = "\t"
        elif len(options.delimiter) != 1:
            parser.error("Specify exactly one character for the --delimiter option: %s" % options.delimiter)
    else:
        if options.delimiter:
            parser.error("--delimiter option is only valid for CSV file formats")
    
    # -
    
    return options

def json_writer(filename, fields, task_queue, error_queue, format):
    try:
        with open(filename, "w") as out:
            first = True
            if format != "ndjson":
                out.write("[")
            item = task_queue.get()
            while not isinstance(item, StopIteration):
                row = item[0]
                if fields is not None:
                    for item in list(row.keys()):
                        if item not in fields:
                            del row[item]
                if first:
                    if format == "ndjson":
                        out.write(json.dumps(row))
                    else:
                        out.write("\n" + json.dumps(row))
                    first = False
                elif format == "ndjson":
                    out.write("\n" + json.dumps(row))
                else:
                    out.write(",\n" + json.dumps(row))

                item = task_queue.get()
            if format != "ndjson":
                out.write("\n]\n")
    except:
        ex_type, ex_class, tb = sys.exc_info()
        error_queue.put((ex_type, ex_class, traceback.extract_tb(tb)))

        # Read until the exit task so the readers do not hang on pushing onto the queue
        while not isinstance(task_queue.get(), StopIteration):
            pass

def csv_writer(filename, fields, delimiter, task_queue, error_queue):
    try:
        with open(filename, "w") as out:
            out_writer = csv.writer(out, delimiter=delimiter)
            out_writer.writerow(fields)

            item = task_queue.get()
            while not isinstance(item, StopIteration):
                row = item[0]
                info = []
                # If the data is a simple type, just write it directly, otherwise, write it as json
                for field in fields:
                    if field not in row:
                        info.append(None)
                    elif isinstance(row[field], numbers.Number):
                        info.append(str(row[field]))
                    elif isinstance(row[field], str):
                        info.append(row[field])
                    elif isinstance(row[field], unicode):
                        info.append(row[field].encode('utf-8'))
                    else:
                        if str == unicode:
                            info.append(json.dumps(row[field]))
                        else:
                            info.append(json.dumps(row[field]).encode('utf-8'))
                out_writer.writerow(info)
                item = task_queue.get()
    except:
        ex_type, ex_class, tb = sys.exc_info()
        error_queue.put((ex_type, ex_class, traceback.extract_tb(tb)))

        # Read until the exit task so the readers do not hang on pushing onto the queue
        while not isinstance(task_queue.get(), StopIteration):
            pass

def export_table(db, table, directory, options, error_queue, progress_info, sindex_counter, exit_event):
    signal.signal(signal.SIGINT, signal.SIG_DFL) # prevent signal handlers from being set in child processes
    
    writer = None

    try:
        # -- get table info
        
        table_info = options.retryQuery('table info: %s.%s' % (db, table), query.db(db).table(table).info())
        
        # Rather than just the index names, store all index information
        table_info['indexes'] = options.retryQuery(
            'table index data %s.%s' % (db, table),
            query.db(db).table(table).index_status(),
            runOptions={'binary_format':'raw'}
        )
        
        with open(os.path.join(directory, db, table + '.info'), 'w') as info_file:
            info_file.write(json.dumps(table_info) + "\n")
        
        with sindex_counter.get_lock():
            sindex_counter.value += len(table_info["indexes"])
        
        # -- start the writer
        
        task_queue = SimpleQueue()
        writer = None
        if options.format == "json":
            filename = directory + "/%s/%s.json" % (db, table)
            writer = multiprocessing.Process(target=json_writer, args=(filename, options.fields, task_queue, error_queue, options.format))
        elif options.format == "csv":
            filename = directory + "/%s/%s.csv" % (db, table)
            writer = multiprocessing.Process(target=csv_writer, args=(filename, options.fields, options.delimiter, task_queue, error_queue))
        elif options.format == "ndjson":
            filename = directory + "/%s/%s.ndjson" % (db, table)
            writer = multiprocessing.Process(target=json_writer, args=(filename, options.fields, task_queue, error_queue, options.format))
        else:
            raise RuntimeError("unknown format type: %s" % options.format)
        writer.start()
        
        # -- read in the data source
        
        # - 
        
        lastPrimaryKey = None
        read_rows      = 0
        runOptions     = {
            "time_format":"raw",
            "binary_format":"raw"
        }
        if options.outdated:
            runOptions["read_mode"] = "outdated"
        cursor = options.retryQuery(
            'inital cursor for %s.%s' % (db, table),
            query.db(db).table(table).order_by(index=table_info["primary_key"]),
            runOptions=runOptions
        )
        while not exit_event.is_set():
            try:
                for row in cursor:
                    # bail on exit
                    if exit_event.is_set():
                        break
                    
                    # add to the output queue
                    task_queue.put([row])
                    lastPrimaryKey = row[table_info["primary_key"]]
                    read_rows += 1
                    
                    # Update the progress every 20 rows
                    if read_rows % 20 == 0:
                        progress_info[0].value = read_rows
                
                else:
                    # Export is done - since we used estimates earlier, update the actual table size
                    progress_info[0].value = read_rows
                    progress_info[1].value = read_rows
                    break
            
            except (errors.ReqlTimeoutError, errors.ReqlDriverError) as e:
                # connection problem, re-setup the cursor
                try:
                    cursor.close()
                except Exception: pass
                cursor = options.retryQuery(
                    'backup cursor for %s.%s' % (db, table),
                    query.db(db).table(table).between(lastPrimaryKey, None, left_bound="open").order_by(index=table_info["primary_key"]),
                    runOptions=runOptions
                )
    
    except (errors.ReqlError, errors.ReqlDriverError) as ex:
        error_queue.put((RuntimeError, RuntimeError(ex.message), traceback.extract_tb(sys.exc_info()[2])))
    except:
        ex_type, ex_class, tb = sys.exc_info()
        error_queue.put((ex_type, ex_class, traceback.extract_tb(tb)))
    finally:
        if writer and writer.is_alive():
            task_queue.put(StopIteration())
            writer.join()

def abort_export(signum, frame, exit_event, interrupt_event):
    interrupt_event.set()
    exit_event.set()

# We sum up the row count from all tables for total percentage completion
#  This is because table exports can be staggered when there are not enough clients
#  to export all of them at once.  As a result, the progress bar will not necessarily
#  move at the same rate for different tables.
def update_progress(progress_info, options):
    rows_done = 0
    total_rows = 1
    for current, max_count in progress_info:
        curr_val = current.value
        max_val = max_count.value
        if curr_val < 0:
            # There is a table that hasn't finished counting yet, we can't report progress
            rows_done = 0
            break
        else:
            rows_done += curr_val
            total_rows += max_val

    if not options.quiet:
        utils_common.print_progress(float(rows_done) / total_rows, indent=4)

def run_clients(options, workingDir, db_table_set):
    # Spawn one client for each db.table, up to options.clients at a time
    exit_event = multiprocessing.Event()
    processes = []
    error_queue = SimpleQueue()
    interrupt_event = multiprocessing.Event()
    sindex_counter = multiprocessing.Value(ctypes.c_longlong, 0)

    signal.signal(signal.SIGINT, lambda a, b: abort_export(a, b, exit_event, interrupt_event))
    errors = []

    try:
        progress_info = []
        arg_lists = []
        for db, table in db_table_set:
            
            tableSize = int(options.retryQuery("count", query.db(db).table(table).info()['doc_count_estimates'].sum()))
            
            progress_info.append((multiprocessing.Value(ctypes.c_longlong, 0),
                                  multiprocessing.Value(ctypes.c_longlong, tableSize)))
            arg_lists.append((db, table,
                              workingDir,
                              options,
                              error_queue,
                              progress_info[-1],
                              sindex_counter,
                              exit_event,
                              ))


        # Wait for all tables to finish
        while processes or arg_lists:
            time.sleep(0.1)

            while not error_queue.empty():
                exit_event.set() # Stop immediately if an error occurs
                errors.append(error_queue.get())

            processes = [process for process in processes if process.is_alive()]

            if len(processes) < options.clients and len(arg_lists) > 0:
                newProcess = multiprocessing.Process(target=export_table, args=arg_lists.pop(0))
                newProcess.start()
                processes.append(newProcess)

            update_progress(progress_info, options)

        # If we were successful, make sure 100% progress is reported
        # (rows could have been deleted which would result in being done at less than 100%)
        if len(errors) == 0 and not interrupt_event.is_set() and not options.quiet:
            utils_common.print_progress(1.0, indent=4)

        # Continue past the progress output line and print total rows processed
        def plural(num, text, plural_text):
            return "%d %s" % (num, text if num == 1 else plural_text)

        if not options.quiet:
            print("\n    %s exported from %s, with %s" %
                  (plural(sum([max(0, info[0].value) for info in progress_info]), "row", "rows"),
                   plural(len(db_table_set), "table", "tables"),
                   plural(sindex_counter.value, "secondary index", "secondary indexes")))
    finally:
        signal.signal(signal.SIGINT, signal.SIG_DFL)

    if interrupt_event.is_set():
        raise RuntimeError("Interrupted")

    if len(errors) != 0:
        # multiprocessing queues don't handle tracebacks, so they've already been stringified in the queue
        for error in errors:
            print("%s" % error[1], file=sys.stderr)
            if options.debug:
                print("%s traceback: %s" % (error[0].__name__, error[2]), file=sys.stderr)
        raise RuntimeError("Errors occurred during export")

def run(options):
    # Make sure this isn't a pre-`reql_admin` cluster - which could result in data loss
    # if the user has a database named 'rethinkdb'
    utils_common.check_minimum_version(options, '1.6')
    
    # get the complete list of tables
    db_table_set = set()
    allTables = [utils_common.DbTable(x['db'], x['name']) for x in options.retryQuery('list tables', query.db('rethinkdb').table('table_config').pluck(['db', 'name']))]
    if not options.db_tables:
        db_table_set = allTables # default to all tables
    else:
        allDatabases = options.retryQuery('list dbs', query.db_list().filter(query.row.ne('rethinkdb')))
        for db_table in options.db_tables:
            db, table = db_table
            assert db != 'rethinkdb', "Error: Cannot export tables from the system database: 'rethinkdb'" # should not be possible
            if db not in allDatabases:
                raise RuntimeError("Error: Database '%s' not found" % db)
            
            if table is None: # This is just a db name, implicitly selecting all tables in that db
                db_table_set.update(set([x for x in allTables if x.db == db]))
            else:
                if utils_common.DbTable(db, table) not in allTables:
                    raise RuntimeError("Error: Table not found: '%s.%s'" % (db, table))
                db_table_set.add(db_table)

    # Determine the actual number of client processes we'll have
    options.clients = min(options.clients, len(db_table_set))
    
    # create the working directory and its structure
    parentDir = os.path.dirname(options.directory)
    if not os.path.exists(parentDir):
        if os.path.isdir(parentDir):
            raise RuntimeError("Output parent directory is not a directory: %s" % (opt, value))
        try:
            os.makedirs(parentDir)
        except OSError as e:
            raise optparse.OptionValueError("Unable to create parent directory for %s: %s (%s)" % (opt, value, e.strerror))
    workingDir = tempfile.mkdtemp(prefix=os.path.basename(options.directory) + '_partial_', dir=os.path.dirname(options.directory))
    try:
        for db in set([db for db, table in db_table_set]):
            os.makedirs(os.path.join(workingDir, str(db)))
    except OSError as e:
        raise RuntimeError("Failed to create temporary directory (%s): %s" % (e.filename, ex.strerror))
    
    # Run the export
    run_clients(options, workingDir, db_table_set)
    
    # Move the temporary directory structure over to the original output directory
    try:
        if os.path.isdir(options.directory):
            os.rmdir(options.directory) # an empty directory is created here when using _dump
        elif os.path.exists(options.directory):
            raise Exception('There was a file at the output location: %s' % options.directory)
        os.rename(workingDir, options.directory)
    except OSError as e:
        raise RuntimeError("Failed to move temporary directory to output directory (%s): %s" % (options.directory, e.strerror))

def main(argv=None, prog=None):
    options = parse_options(argv or sys.argv[1:], prog=prog)
    
    start_time = time.time()
    try:
        run(options)
    except Exception as ex:
        if options.debug:
            traceback.print_exc()
        print(ex, file=sys.stderr)
        return 1
    if not options.quiet:
        print("  Done (%.2f seconds)" % (time.time() - start_time))
    return 0

if __name__ == "__main__":
    sys.exit(main())
