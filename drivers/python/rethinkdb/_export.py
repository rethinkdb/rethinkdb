#!/usr/bin/env python
from __future__ import print_function

import signal

# When running a subprocess, we may inherit the signal handler - remove it
signal.signal(signal.SIGINT, signal.SIG_DFL)

import sys, os, datetime, time, json, traceback, csv
import multiprocessing, subprocess, re, ctypes, numbers
from optparse import OptionParser
from ._backup import *
import rethinkdb as r

try:
    unicode
except NameError:
    unicode = str
try:
    long
except NameError:
    long = int
try:
    from multiprocessing import SimpleQueue
except ImportError:
    from multiprocessing.queues import SimpleQueue


info = "'rethinkdb export` exports data from a RethinkDB cluster into a directory"
usage = "\
  rethinkdb export [-c HOST:PORT] [-a AUTH_KEY] [-d DIR] [-e (DB | DB.TABLE)]...\n\
      [--format (csv | json | nsj)] [--fields FIELD,FIELD...] [--delimiter CHARACTER]\n\
      [--clients NUM]"

def print_export_help():
    print(info)
    print(usage)
    print("")
    print("  -h [ --help ]                    print this help")
    print("  -c [ --connect ] HOST:PORT       host and client port of a rethinkdb node to connect")
    print("                                   to (defaults to localhost:28015)")
    print("  -a [ --auth ] AUTH_KEY           authorization key for rethinkdb clients")
    print("  -d [ --directory ] DIR           directory to output to (defaults to")
    print("                                   rethinkdb_export_DATE_TIME)")
    print("  --format (csv | json | nsj)      format to write (defaults to json)")
    print("  --fields FIELD,FIELD...          limit the exported fields to those specified")
    print("                                   (required for CSV format)")
    print("  -e [ --export ] (DB | DB.TABLE)  limit dump to the given database or table (may")
    print("                                   be specified multiple times)")
    print("  --clients NUM                    number of tables to export simultaneously (defaults")
    print("                                   to 3)")
    print("")
    print("Export in CSV format:")
    print("  --delimiter CHARACTER            character to be used as field delimiter, or '\\t' for tab")
    print("")
    print("EXAMPLES:")
    print("rethinkdb export -c mnemosyne:39500")
    print("  Export all data from a cluster running on host 'mnemosyne' with a client port at 39500.")
    print("")
    print("rethinkdb export -e test -d rdb_export")
    print("  Export only the 'test' database on a local cluster into a named directory.")
    print("")
    print("rethinkdb export -c hades -e test.subscribers -a hunter2")
    print("  Export a specific table from a cluster running on host 'hades' which requires authorization.")
    print("")
    print("rethinkdb export --format csv -e test.history --fields time,message --delimiter ';'")
    print("  Export a specific table from a local cluster in CSV format with the fields 'time' and 'message',")
    print("  using a semicolon as field delimiter (rather than a comma).")
    print("")
    print("rethinkdb export --fields id,value -e test.data")
    print("  Export a specific table from a local cluster in JSON format with only the fields 'id' and 'value'.")

def parse_options():
    parser = OptionParser(add_help_option=False, usage=usage)
    parser.add_option("-c", "--connect", dest="host", metavar="HOST:PORT", default="localhost:28015", type="string")
    parser.add_option("-a", "--auth", dest="auth_key", metavar="AUTHKEY", default="", type="string")
    parser.add_option("--format", dest="format", metavar="json | csv | nsj", default="json", type="string")
    parser.add_option("-d", "--directory", dest="directory", metavar="DIRECTORY", default=None, type="string")
    parser.add_option("-e", "--export", dest="tables", metavar="DB | DB.TABLE", default=[], action="append", type="string")
    parser.add_option("--fields", dest="fields", metavar="<FIELD>,<FIELD>...", default=None, type="string")
    parser.add_option("--delimiter", dest="delimiter", metavar="CHARACTER", default=None, type="string")
    parser.add_option("--clients", dest="clients", metavar="NUM", default=3, type="int")
    parser.add_option("-h", "--help", dest="help", default=False, action="store_true")
    parser.add_option("--debug", dest="debug", default=False, action="store_true")
    (options, args) = parser.parse_args()

    # Check validity of arguments
    if len(args) != 0:
        raise RuntimeError("Error: No positional arguments supported. Unrecognized option '%s'" % args[0])

    if options.help:
        print_export_help()
        exit(0)

    res = {}

    # Verify valid host:port --connect option
    (res["host"], res["port"]) = parse_connect_option(options.host)

    # Verify valid --format option
    if options.format not in ["csv", "json", "nsj"]:
        raise RuntimeError("Error: Unknown format '%s', valid options are 'csv', 'json', and 'nsj'" % options.format)
    res["format"] = options.format

    # Verify valid directory option
    if options.directory is None:
        dirname = "./rethinkdb_export_%s" % datetime.datetime.today().strftime("%Y-%m-%dT%H:%M:%S")
    else:
        dirname = options.directory
    res["directory"] = os.path.abspath(dirname)
    res["directory_partial"] = os.path.abspath(dirname + "_part")

    if os.path.exists(res["directory"]):
        raise RuntimeError("Error: Output directory already exists: %s" % res["directory"])
    if os.path.exists(res["directory_partial"]):
        raise RuntimeError("Error: Partial output directory already exists: %s" % res["directory_partial"])

    # Verify valid --export options
    res["db_tables"] = parse_db_table_options(options.tables)

    # Parse fields
    if options.fields is None:
        if options.format == "csv":
            raise RuntimeError("Error: Cannot write a CSV with no fields selected.  The '--fields' option must be specified.")
        res["fields"] = None
    elif len(res["db_tables"]) != 1 or res["db_tables"][0][1] is None:
        raise RuntimeError("Error: Can only use the --fields option when exporting a single table")
    else:
        res["fields"] = options.fields.split(",")

    if options.delimiter is None:
        res["delimiter"] = ","

    else:

        if options.format != "csv":
            raise RuntimeError("Error: --delimiter option is only valid for CSV file formats")

        if len(options.delimiter) == 1:
            res["delimiter"] = options.delimiter

        elif options.delimiter == "\\t":
            res["delimiter"] = "\t"

        else:
            raise RuntimeError("Error: Must specify only one character for the --delimiter option")

    # Get number of clients
    if options.clients < 1:
        raise RuntimeError("Error: invalid number of clients (%d), must be greater than zero" % options.clients)
    res["clients"] = options.clients

    res["auth_key"] = options.auth_key
    res["debug"] = options.debug
    return res

# This is called through rdb_call_wrapper and may be called multiple times if
# connection errors occur.  Don't bother setting progress, because this is a
# fairly small operation.
def get_tables(progress, conn, tables):
    dbs = r.db_list().filter(r.row.ne('rethinkdb')).run(conn)
    res = []

    if len(tables) == 0:
        tables = [(db, None) for db in dbs]

    for db_table in tables:
        if db_table[0] == 'rethinkdb':
            raise RuntimeError("Error: Cannot export tables from the system database: 'rethinkdb'")
        if db_table[0] not in dbs:
            raise RuntimeError("Error: Database '%s' not found" % db_table[0])

        if db_table[1] is None: # This is just a db name
            res.extend([(db_table[0], table) for table in r.db(db_table[0]).table_list().run(conn)])
        else: # This is db and table name
            if db_table[1] not in r.db(db_table[0]).table_list().run(conn):
                raise RuntimeError("Error: Table not found: '%s.%s'" % tuple(db_table))
            res.append(tuple(db_table))

    # Remove duplicates by making results a set
    return set(res)

# Make sure the output directory doesn't exist and create the temporary directory structure
def prepare_directories(base_path, base_path_partial, db_table_set):
    os_call_wrapper(lambda x: os.makedirs(x), base_path_partial, "Failed to create temporary directory (%s): %s")

    db_set = set([db for db, table in db_table_set])
    for db in db_set:
        db_dir = base_path_partial + "/%s" % db
        os_call_wrapper(lambda x: os.makedirs(x), db_dir, "Failed to create temporary directory (%s): %s")

# Move the temporary directory structure over to the original output directory
def finalize_directory(base_path, base_path_partial):
    os_call_wrapper(lambda x: os.rename(base_path_partial, x), base_path,
                    "Failed to move temporary directory to output directory (%s): %s")

# This is called through rdb_call_wrapper and may be called multiple times if
# connection errors occur.  Don't bother setting progress, because we either
# succeed or fail, there is no partial success.
def write_table_metadata(progress, conn, db, table, base_path):
    table_info = r.db(db).table(table).info().run(conn)

    # Rather than just the index names, store all index information
    table_info['indexes'] = r.db(db).table(table).index_status().run(conn, binary_format='raw')

    out = open(base_path + "/%s/%s.info" % (db, table), "w")
    out.write(json.dumps(table_info) + "\n")
    out.close()
    return table_info

# This is called through rdb_call_wrapper and may be called multiple times if
# connection errors occur.  In order to facilitate this, we do an order_by by the
# primary key so that we only ever get a given row once.
def read_table_into_queue(progress, conn, db, table, pkey, task_queue, progress_info, exit_event):
    read_rows = 0
    if progress[0] is None:
        cursor = r.db(db).table(table).order_by(index=pkey).run(conn, time_format="raw", binary_format='raw')
    else:
        cursor = r.db(db).table(table).between(progress[0], None, left_bound="open").order_by(index=pkey).run(conn, time_format="raw", binary_format='raw')

    try:
        for row in cursor:
            if exit_event.is_set():
                break
            task_queue.put([row])

            # Set progress so we can continue from this point if a connection error occurs
            progress[0] = row[pkey]

            # Update the progress every 20 rows - to reduce locking overhead
            read_rows += 1
            if read_rows % 20 == 0:
                progress_info[0].value += 20
    finally:
        progress_info[0].value += read_rows % 20

    # Export is done - since we used estimates earlier, update the actual table size
    progress_info[1].value = progress_info[0].value

def json_writer(filename, fields, task_queue, error_queue, format):
    try:
        with open(filename, "w") as out:
            first = True
            if format != "nsj":
                out.write("[")
            item = task_queue.get()
            while not isinstance(item, StopIteration):
                row = item[0]
                if fields is not None:
                    for item in list(row.keys()):
                        if item not in fields:
                            del row[item]
                if first or format == "nsj":
                    first = False
                    out.write("\n" + json.dumps(row))
                else:
                    out.write(",\n" + json.dumps(row))

                item = task_queue.get()
            if format != "nsj":
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
                        info.append(json.dumps(row[field]))
                out_writer.writerow(info)
                item = task_queue.get()
    except:
        ex_type, ex_class, tb = sys.exc_info()
        error_queue.put((ex_type, ex_class, traceback.extract_tb(tb)))

        # Read until the exit task so the readers do not hang on pushing onto the queue
        while not isinstance(task_queue.get(), StopIteration):
            pass

def launch_writer(format, directory, db, table, fields, delimiter, task_queue, error_queue):
    if format == "json":
        filename = directory + "/%s/%s.json" % (db, table)
        return multiprocessing.Process(target=json_writer,
                                       args=(filename, fields, task_queue, error_queue))
    elif format == "csv":
        filename = directory + "/%s/%s.csv" % (db, table)
        return multiprocessing.Process(target=csv_writer,
                                       args=(filename, fields, delimiter, task_queue, error_queue))

    elif format == "nsj":
       filename = directory + "/%s/%s.json" % (db, table)
       return multiprocessing.Process(target=json_writer,
                                      args=(filename, fields, task_queue, error_queue, format))

    else:
        raise RuntimeError("unknown format type: %s" % format)

def get_table_size(progress, conn, db, table, progress_info):
    table_size = r.db(db).table(table).info()['doc_count_estimates'].sum().run(conn)
    progress_info[1].value = int(table_size)
    progress_info[0].value = 0

def export_table(host, port, auth_key, db, table, directory, fields, delimiter, format,
                 error_queue, progress_info, sindex_counter, stream_semaphore, exit_event):
    writer = None

    try:
        # This will open at least one connection for each rdb_call_wrapper, which is
        # a little wasteful, but shouldn't be a big performance hit
        conn_fn = lambda: r.connect(host, port, auth_key=auth_key)
        rdb_call_wrapper(conn_fn, "count", get_table_size, db, table, progress_info)
        table_info = rdb_call_wrapper(conn_fn, "info", write_table_metadata, db, table, directory)
        sindex_counter.value += len(table_info["indexes"])

        with stream_semaphore:
            task_queue = SimpleQueue()
            writer = launch_writer(format, directory, db, table, fields, delimiter, task_queue, error_queue)
            writer.start()

            rdb_call_wrapper(conn_fn, "table scan", read_table_into_queue, db, table,
                             table_info["primary_key"], task_queue, progress_info, exit_event)
    except (r.ReqlError, r.ReqlDriverError) as ex:
        error_queue.put((RuntimeError, RuntimeError(ex.message), traceback.extract_tb(sys.exc_info()[2])))
    except:
        ex_type, ex_class, tb = sys.exc_info()
        error_queue.put((ex_type, ex_class, traceback.extract_tb(tb)))
    finally:
        if writer is not None and writer.is_alive():
            task_queue.put(StopIteration())
            writer.join()

def abort_export(signum, frame, exit_event, interrupt_event):
    interrupt_event.set()
    exit_event.set()

# We sum up the row count from all tables for total percentage completion
#  This is because table exports can be staggered when there are not enough clients
#  to export all of them at once.  As a result, the progress bar will not necessarily
#  move at the same rate for different tables.
def update_progress(progress_info):
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

    print_progress(float(rows_done) / total_rows)

def run_clients(options, db_table_set):
    # Spawn one client for each db.table
    exit_event = multiprocessing.Event()
    processes = []
    error_queue = SimpleQueue()
    interrupt_event = multiprocessing.Event()
    stream_semaphore = multiprocessing.BoundedSemaphore(options["clients"])
    sindex_counter = multiprocessing.Value(ctypes.c_longlong, 0)

    signal.signal(signal.SIGINT, lambda a, b: abort_export(a, b, exit_event, interrupt_event))
    errors = [ ]

    try:
        progress_info = []

        for db, table in db_table_set:
            progress_info.append((multiprocessing.Value(ctypes.c_longlong, -1),
                                  multiprocessing.Value(ctypes.c_longlong, 0)))
            processes.append(multiprocessing.Process(target=export_table,
                                                     args=(options["host"],
                                                           options["port"],
                                                           options["auth_key"],
                                                           db, table,
                                                           options["directory_partial"],
                                                           options["fields"],
                                                           options["delimiter"],
                                                           options["format"],
                                                           error_queue,
                                                           progress_info[-1],
                                                           sindex_counter,
                                                           stream_semaphore,
                                                           exit_event)))
            processes[-1].start()

        # Wait for all tables to finish
        while len(processes) > 0:
            time.sleep(0.1)
            while not error_queue.empty():
                exit_event.set() # Stop rather immediately if an error occurs
                errors.append(error_queue.get())
            processes = [process for process in processes if process.is_alive()]
            update_progress(progress_info)

        # If we were successful, make sure 100% progress is reported
        # (rows could have been deleted which would result in being done at less than 100%)
        if len(errors) == 0 and not interrupt_event.is_set():
            print_progress(1.0)

        # Continue past the progress output line and print total rows processed
        def plural(num, text, plural_text):
            return "%d %s" % (num, text if num == 1 else plural_text)

        print("")
        print("%s exported from %s, with %s" %
              (plural(sum([max(0, info[0].value) for info in progress_info]), "row", "rows"),
               plural(len(db_table_set), "table", "tables"),
               plural(sindex_counter.value, "secondary index", "secondary indexes")))
    finally:
        signal.signal(signal.SIGINT, signal.SIG_DFL)

    if interrupt_event.is_set():
        raise RuntimeError("Interrupted")

    if len(errors) != 0:
        # multiprocessing queues don't handling tracebacks, so they've already been stringified in the queue
        for error in errors:
            print("%s" % error[1], file=sys.stderr)
            if options["debug"]:
                print("%s traceback: %s" % (error[0].__name__, error[2]), file=sys.stderr)
        raise RuntimeError("Errors occurred during export")

def main():
    try:
        options = parse_options()
    except RuntimeError as ex:
        print("Usage:\n%s" % usage, file=sys.stderr)
        print(ex, file=sys.stderr)
        return 1

    try:
        conn_fn = lambda: r.connect(options["host"], options["port"], auth_key=options["auth_key"])
        # Make sure this isn't a pre-`reql_admin` cluster - which could result in data loss
        # if the user has a database named 'rethinkdb'
        rdb_call_wrapper(conn_fn, "version check", check_minimum_version, (1, 16, 0))
        db_table_set = rdb_call_wrapper(conn_fn, "table list", get_tables, options["db_tables"])
        del options["db_tables"] # This is not needed anymore, db_table_set is more useful

        # Determine the actual number of client processes we'll have
        options["clients"] = min(options["clients"], len(db_table_set))

        prepare_directories(options["directory"], options["directory_partial"], db_table_set)
        start_time = time.time()
        run_clients(options, db_table_set)
        finalize_directory(options["directory"], options["directory_partial"])
    except RuntimeError as ex:
        print(ex, file=sys.stderr)
        return 1
    print("  Done (%d seconds)" % (time.time() - start_time))
    return 0

if __name__ == "__main__":
    exit(main())
