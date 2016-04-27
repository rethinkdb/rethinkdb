#!/usr/bin/env python

from __future__ import print_function

import csv, ctypes, datetime, json, multiprocessing, numbers, optparse
import os, re, signal, sys, time, traceback

from . import utils_common, net
r = utils_common.r

# When running a subprocess, we may inherit the signal handler - remove it
signal.signal(signal.SIGINT, signal.SIG_DFL)

try:
    unicode
except NameError:
    unicode = str
try:
    from multiprocessing import SimpleQueue
except ImportError:
    from multiprocessing.queues import SimpleQueue


info = "'rethinkdb export` exports data from a RethinkDB cluster into a directory"
usage = "\
  rethinkdb export [-c HOST:PORT] [-p] [--password-file FILENAME] [--tls-cert filename] [-d DIR] [-e (DB | DB.TABLE)]...\n\
      [--format (csv | json | ndjson)] [--fields FIELD,FIELD...] [--delimiter CHARACTER]\n\
      [--clients NUM]"

def print_export_help():
    print(info)
    print(usage)
    print("")
    print("  -h [ --help ]                    print this help")
    print("  -c [ --connect ] HOST:PORT       host and client port of a rethinkdb node to connect")
    print("                                   to (defaults to localhost:%d)" % net.DEFAULT_PORT)
    print("  --tls-cert FILENAME              certificate file to use for TLS encryption.")
    print("  -p [ --password ]                interactively prompt for a password required to connect.")
    print("  --password-file FILENAME         read password required to connect from file.")
    print("  -d [ --directory ] DIR           directory to output to (defaults to")
    print("                                   rethinkdb_export_DATE_TIME)")
    print("  --format (csv | json | ndjson)   format to write (defaults to json.")
    print("                                   ndjson is newline delimited json.)")
    print("  --fields FIELD,FIELD...          limit the exported fields to those specified")
    print("                                   (required for CSV format)")
    print("  -e [ --export ] (DB | DB.TABLE)  limit dump to the given database or table (may")
    print("                                   be specified multiple times)")
    print("  --clients NUM                    number of tables to export simultaneously (defaults")
    print("                                   to 3)")
    print("  -q [ --quiet ]                   suppress non-error messages")
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
    print("rethinkdb export -c hades -e test.subscribers -p")
    print("  Export a specific table from a cluster running on host 'hades' which requires a password.")
    print("")
    print("rethinkdb export --format csv -e test.history --fields time,message --delimiter ';'")
    print("  Export a specific table from a local cluster in CSV format with the fields 'time' and 'message',")
    print("  using a semicolon as field delimiter (rather than a comma).")
    print("")
    print("rethinkdb export --fields id,value -e test.data")
    print("  Export a specific table from a local cluster in JSON format with only the fields 'id' and 'value'.")

def parse_options(argv):
    parser = optparse.OptionParser(add_help_option=False, usage=usage)
    parser.add_option("-c", "--connect", dest="host", metavar="HOST:PORT")
    parser.add_option("--format", dest="format", metavar="json | csv | ndjson", default="json")
    parser.add_option("-d", "--directory", dest="directory", metavar="DIRECTORY", default=None)
    parser.add_option("-e", "--export", dest="tables", metavar="DB | DB.TABLE", default=[], action="append")
    parser.add_option("--tls-cert", dest="tls_cert", metavar="TLS_CERT", default="")
    parser.add_option("--fields", dest="fields", metavar="<FIELD>,<FIELD>...", default=None)
    parser.add_option("--delimiter", dest="delimiter", metavar="CHARACTER", default=None)
    parser.add_option("--clients", dest="clients", metavar="NUM", default=3, type="int")
    parser.add_option("-h", "--help", dest="help", default=False, action="store_true")
    parser.add_option("-q", "--quiet", dest="quiet", default=False, action="store_true")
    parser.add_option("--debug", dest="debug", default=False, action="store_true")
    parser.add_option("-p", "--password", dest="password", default=False, action="store_true")
    parser.add_option("--password-file", dest="password_file", default=None)
    options, args = parser.parse_args(argv)

    # Check validity of arguments
    if len(args) != 0:
        raise RuntimeError("Error: No positional arguments supported. Unrecognized option '%s'" % args[0])

    if options.help:
        print_export_help()
        exit(0)

    res = {}

    # Verify valid host:port --connect option
    (res["host"], res["port"]) = utils_common.parse_connect_option(options.host)

    res["tls_cert"] = utils_common.ssl_option(options.tls_cert)

    # Verify valid --format option
    if options.format not in ["csv", "json", "ndjson"]:
        raise RuntimeError("Error: Unknown format '%s', valid options are 'csv', 'json', and 'ndjson'" % options.format)
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
    res["db_tables"] = utils_common.parse_db_table_options(options.tables)

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

    res["quiet"] = options.quiet
    res["debug"] = options.debug

    res["password"] = options.password
    res["password-file"] = options.password_file
    return res

# This is called through utils_common.rdb_call_wrapper and may be called multiple times if
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

def os_call_wrapper(fn, filename, error_str):
    try:
        fn(filename)
    except OSError as ex:
        raise RuntimeError(error_str % (filename, ex.strerror))

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

# This is called through utils_common.rdb_call_wrapper and may be called multiple times if
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

# This is called through utils_common.rdb_call_wrapper and may be called multiple times if
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
                        info.append(json.dumps(row[field]))
                out_writer.writerow(info)
                item = task_queue.get()
    except:
        ex_type, ex_class, tb = sys.exc_info()
        error_queue.put((ex_type, ex_class, traceback.extract_tb(tb)))

        # Read until the exit task so the readers do not hang on pushing onto the queue
        while not isinstance(task_queue.get(), StopIteration):
            pass

def get_all_table_sizes(host, port, db_table_set, ssl_op, admin_password):
    def get_table_size(progress, conn, db, table):
        return r.db(db).table(table).info()['doc_count_estimates'].sum().run(conn)

    conn_fn = lambda: r.connect(host,
                                port,
                                ssl=ssl_op,
                                user="admin",
                                password=admin_password)

    ret = dict()
    for pair in db_table_set:
        db, table = pair
        ret[pair] = int(utils_common.rdb_call_wrapper(conn_fn, "count", get_table_size, db, table))

    return ret

def export_table(host, port, db, table, directory, fields, delimiter, format,
                 error_queue, progress_info, sindex_counter, exit_event, ssl_op, admin_password):
    writer = None

    try:
        # This will open at least one connection for each utils_common.rdb_call_wrapper, which is
        # a little wasteful, but shouldn't be a big performance hit
        conn_fn = lambda: r.connect(host,
                                    port,
                                    ssl=ssl_op,
                                    user="admin",
                                    password=admin_password)
        table_info = utils_common.rdb_call_wrapper(conn_fn, "info", write_table_metadata, db, table, directory)
        sindex_counter.value += len(table_info["indexes"])
        
        # -- start the writer
        
        task_queue = SimpleQueue()
        writer = None
        if format == "json":
            filename = directory + "/%s/%s.json" % (db, table)
            writer = multiprocessing.Process(target=json_writer,
                                           args=(filename, fields, task_queue, error_queue, format))
        elif format == "csv":
            filename = directory + "/%s/%s.csv" % (db, table)
            writer = multiprocessing.Process(target=csv_writer,
                                           args=(filename, fields, delimiter, task_queue, error_queue))
        elif format == "ndjson":
            filename = directory + "/%s/%s.ndjson" % (db, table)
            writer = multiprocessing.Process(target=json_writer,
                                           args=(filename, fields, task_queue, error_queue, format))
        else:
            raise RuntimeError("unknown format type: %s" % format)
        writer.start()
        
        # -- read in the data source
        
        utils_common.rdb_call_wrapper(conn_fn, "table scan", read_table_into_queue, db, table,
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

    if not options["quiet"]:
        utils_common.print_progress(float(rows_done) / total_rows)

def run_clients(options, db_table_set, admin_password):
    # Spawn one client for each db.table
    exit_event = multiprocessing.Event()
    processes = []
    error_queue = SimpleQueue()
    interrupt_event = multiprocessing.Event()
    sindex_counter = multiprocessing.Value(ctypes.c_longlong, 0)

    signal.signal(signal.SIGINT, lambda a, b: abort_export(a, b, exit_event, interrupt_event))
    errors = [ ]

    try:
        sizes = get_all_table_sizes(options["host"], options["port"], db_table_set, options["tls_cert"], admin_password)

        progress_info = []

        arg_lists = []
        for db, table in db_table_set:
            progress_info.append((multiprocessing.Value(ctypes.c_longlong, 0),
                                  multiprocessing.Value(ctypes.c_longlong, sizes[(db, table)])))
            arg_lists.append((options["host"],
                              options["port"],
                              db, table,
                              options["directory_partial"],
                              options["fields"],
                              options["delimiter"],
                              options["format"],
                              error_queue,
                              progress_info[-1],
                              sindex_counter,
                              exit_event,
                              options["tls_cert"],
                              admin_password))


        # Wait for all tables to finish
        while len(processes) > 0 or len(arg_lists) > 0:
            time.sleep(0.1)

            while not error_queue.empty():
                exit_event.set() # Stop immediately if an error occurs
                errors.append(error_queue.get())

            processes = [process for process in processes if process.is_alive()]

            if len(processes) < options["clients"] and len(arg_lists) > 0:
                processes.append(multiprocessing.Process(target=export_table,
                                                         args=arg_lists.pop(0)))
                processes[-1].start()

            update_progress(progress_info, options)

        # If we were successful, make sure 100% progress is reported
        # (rows could have been deleted which would result in being done at less than 100%)
        if len(errors) == 0 and not interrupt_event.is_set() and not options["quiet"]:
            utils_common.print_progress(1.0)

        # Continue past the progress output line and print total rows processed
        def plural(num, text, plural_text):
            return "%d %s" % (num, text if num == 1 else plural_text)

        if not options["quiet"]:
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

def main(argv=None):
    if argv is None:
        argv = sys.argv[1:]
    try:
        options = parse_options(argv)
    except RuntimeError as ex:
        print("Usage:\n%s" % usage, file=sys.stderr)
        print(ex, file=sys.stderr)
        return 1

    try:
        admin_password = utils_common.get_password(options["password"], options["password-file"])
        conn_fn = lambda: r.connect(options["host"],
                                    options["port"],
                                    ssl=options["tls_cert"],
                                    user="admin",
                                    password=admin_password)
        # Make sure this isn't a pre-`reql_admin` cluster - which could result in data loss
        # if the user has a database named 'rethinkdb'
        utils_common.rdb_call_wrapper(conn_fn, "version check", utils_common.check_minimum_version, (1, 16, 0))
        db_table_set = utils_common.rdb_call_wrapper(conn_fn, "table list", get_tables, options["db_tables"])
        del options["db_tables"] # This is not needed anymore, db_table_set is more useful

        # Determine the actual number of client processes we'll have
        options["clients"] = min(options["clients"], len(db_table_set))

        prepare_directories(options["directory"], options["directory_partial"], db_table_set)
        start_time = time.time()
        run_clients(options, db_table_set, admin_password)
        finalize_directory(options["directory"], options["directory_partial"])
    except RuntimeError as ex:
        print(ex, file=sys.stderr)
        return 1
    if not options["quiet"]:
        print("  Done (%d seconds)" % (time.time() - start_time))
    return 0

if __name__ == "__main__":
    exit(main())
