#!/usr/bin/env python
import signal

import sys, os, datetime, time, copy, json, traceback, csv, cPickle, string
import multiprocessing, multiprocessing.queues, subprocess, re, ctypes
from optparse import OptionParser

try:
    import rethinkdb as r
except ImportError:
    print "The RethinkDB python driver is required to use this command."
    print "Please install the driver via `pip install rethinkdb`."
    exit(1)

info = "'rethinkdb import` loads data into a RethinkDB cluster"
usage = "\
  rethinkdb import -d DIR [-c HOST:PORT] [-a AUTH_KEY] [--force]\n\
      [-i (DB | DB.TABLE)] [--clients NUM]\n\
  rethinkdb import -f FILE --table DB.TABLE [-c HOST:PORT] [-a AUTH_KEY]\n\
      [--force] [--clients NUM] [--format (csv | json)] [--pkey PRIMARY_KEY]\n\
      [--delimiter CHARACTER] [--custom-header FIELD,FIELD... [--no-header]]"

def print_import_help():
    print info
    print usage
    print ""
    print "  -h [ --help ]                    print this help"
    print "  -c [ --connect ] HOST:PORT       host and client port of a rethinkdb node to connect"
    print "                                   to (defaults to localhost:28015)"
    print "  -a [ --auth ] AUTH_KEY           authorization key for rethinkdb clients"
    print "  --clients NUM_CLIENTS            the number of client connections to use (defaults"
    print "                                   to 8)"
    print "  --force                          import data even if a table already exists, and"
    print "                                   overwrite duplicate primary keys"
    print "  --fields                         limit which fields to use when importing one table"
    print ""
    print "Import directory:"
    print "  -d [ --directory ] DIR           the directory to import data from"
    print "  -i [ --import ] (DB | DB.TABLE)  limit restore to the given database or table (may"
    print "                                   be specified multiple times)"
    print ""
    print "Import file:"
    print "  -f [ --file ] FILE               the file to import data from"
    print "  --table DB.TABLE                 the table to import the data into"
    print "  --format (csv | json)            the format of the file (defaults to json)"
    print "  --pkey PRIMARY_KEY               the field to use as the primary key in the table"
    print ""
    print "Import CSV format:"
    print "  --delimiter CHARACTER            character separating fields, or '\\t' for tab"
    print "  --no-header                      do not read in a header of field names"
    print "  --custom-header FIELD,FIELD...   header to use (overriding file header), must be"
    print "                                   specified if --no-header"
    print ""
    print "EXAMPLES:"
    print ""
    print "rethinkdb import -d rdb_export -c mnemosyne:39500 --clients 128"
    print "  Import data into a cluster running on host 'mnemosyne' with a client port at 39500,"
    print "  using 128 client connections and the named export directory."
    print ""
    print "rethinkdb import -f site_history.csv --format csv --table test.history --pkey count"
    print "  Import data into a local cluster and the table 'history' in the 'test' database,"
    print "  using the named CSV file, and using the 'count' field as the primary key."
    print ""
    print "rethinkdb import -d rdb_export -c hades -a hunter2 -i test"
    print "  Import data into a cluster running on host 'hades' which requires authorization,"
    print "  using only the database 'test' from the named export directory."
    print ""
    print "rethinkdb import -f subscriber_info.json --fields id,name,hashtag --force"
    print "  Import data into a local cluster using the named JSON file, and only the fields"
    print "  'id', 'name', and 'hashtag', overwriting any existing rows with the same primary key."
    print ""
    print "rethinkdb import -f user_data.csv --delimiter ';' --no-header --custom-header id,name,number"
    print "  Import data into a local cluster using the named CSV file with no header and instead"
    print "  use the fields 'id', 'name', and 'number', the delimiter is a semicolon (rather than"
    print "  a comma)."

def parse_options():
    parser = OptionParser(add_help_option=False, usage=usage)
    parser.add_option("-c", "--connect", dest="host", metavar="HOST:PORT", default="localhost:28015", type="string")
    parser.add_option("-a", "--auth", dest="auth_key", metavar="AUTHKEY", default="", type="string")
    parser.add_option("--fields", dest="fields", metavar="FIELD,FIELD...", default=None, type="string")
    parser.add_option("--clients", dest="clients", metavar="NUM_CLIENTS", default=8, type="int")
    parser.add_option("--force", dest="force", action="store_true", default=False)

    # Directory import options
    parser.add_option("-d", "--directory", dest="directory", metavar="DIRECTORY", default=None, type="string")
    parser.add_option("-i", "--import", dest="tables", metavar="DB | DB.TABLE", default=[], action="append", type="string")

    # File import options
    parser.add_option("-f", "--file", dest="import_file", metavar="FILE", default=None, type="string")
    parser.add_option("--format", dest="import_format", metavar="json | csv", default=None, type="string")
    parser.add_option("--table", dest="import_table", metavar="DB.TABLE", default=None, type="string")
    parser.add_option("--pkey", dest="primary_key", metavar="KEY", default = None, type="string")
    parser.add_option("--delimiter", dest="delimiter", metavar="CHARACTER", default = None, type="string")
    parser.add_option("--no-header", dest="no_header", action="store_true", default = False)
    parser.add_option("--custom-header", dest="custom_header", metavar="FIELD,FIELD...", default = None, type="string")
    parser.add_option("-h", "--help", dest="help", default=False, action="store_true")
    (options, args) = parser.parse_args()

    # Check validity of arguments
    if len(args) != 0:
        raise RuntimeError("Error: No positional arguments supported. Unrecognized option '%s'" % args[0])

    if options.help:
        print_import_help()
        exit(0)

    res = { }

    # Verify valid host:port --connect option
    host_port = options.host.split(":")
    if len(host_port) == 1:
        host_port = (host_port[0], "28015") # If just a host, use the default port
    if len(host_port) != 2:
        raise RuntimeError("Error: Invalid 'host:port' format: %s" % options.host)
    (res["host"], res["port"]) = host_port

    if options.clients < 1:
        raise RuntimeError("Error: --client option too low, must have at least one client connection")

    res["auth_key"] = options.auth_key
    res["clients"] = options.clients
    res["force"] = options.force

    # Default behavior for csv files - may be changed by options
    res["delimiter"] = ","
    res["no_header"] = False
    res["custom_header"] = None

    if options.directory is not None:
        # Directory mode, verify directory import options
        if options.import_file is not None:
            raise RuntimeError("Error: --file option is not valid when importing a directory")
        if options.import_format is not None:
            raise RuntimeError("Error: --format option is not valid when importing a directory")
        if options.import_table is not None:
            raise RuntimeError("Error: --table option is not valid when importing a directory")
        if options.primary_key is not None:
            raise RuntimeError("Error: --pkey option is not valid when importing a directory")
        if options.delimiter is not None:
            raise RuntimeError("Error: --delimiter option is not valid when importing a directory")
        if options.no_header is not False:
            raise RuntimeError("Error: --no-header option is not valid when importing a directory")
        if options.custom_header is not None:
            raise RuntimeError("Error: --custom-header option is not valid when importing a directory")

        # Verify valid directory option
        dirname = options.directory
        res["directory"] = os.path.abspath(dirname)

        if not os.path.exists(res["directory"]):
            raise RuntimeError("Error: Directory to import does not exist: %d" % res["directory"])

        # Verify valid --import options
        res["dbs"] = []
        res["tables"] = []
        for item in options.tables:
            if not all(c in string.ascii_letters + string.digits + "._" for c in item):
                raise RuntimeError("Error: Invalid 'db' or 'db.table' name: %s" % item)
            db_table = item.split(".")
            if len(db_table) == 1:
                res["dbs"].append(db_table[0])
            elif len(db_table) == 2:
                res["tables"].append(tuple(db_table))
            else:
                raise RuntimeError("Error: Invalid 'db' or 'db.table' format: %s" % item)

        # Parse fields
        if options.fields is None:
            res["fields"] = None
        elif len(res["dbs"]) != 0 or len(res["tables"] != 1):
            raise RuntimeError("Error: Can only use the --fields option when importing a single table")
        else:
            res["fields"] = options.fields.split(",")

    elif options.import_file is not None:
        # Single file mode, verify file import options
        if len(options.tables) != 0:
            raise RuntimeError("Error: --import option is not valid when importing a single file")
        if options.directory is not None:
            raise RuntimeError("Error: --directory option is not valid when importing a single file")

        import_file = options.import_file
        res["import_file"] = os.path.abspath(import_file)

        if not os.path.exists(res["import_file"]):
            raise RuntimeError("Error: File to import does not exist: %s" % res["import_file"])

        # Verify valid --format option
        if options.import_format is None:
            res["import_format"] = "json"
        elif options.import_format not in ["csv", "json"]:
            raise RuntimeError("Error: Unknown format '%s', valid options are 'csv' and 'json'" % options.import_format)
        else:
            res["import_format"] = options.import_format

        # Verify valid --table option
        if options.import_table is None:
            raise RuntimeError("Error: Must specify a destination table to import into using the --table option")
        if not all(c in string.ascii_letters + string.digits + "._" for c in options.import_table):
            raise RuntimeError("Error: Invalid 'db' or 'db.table' name: %s" % options.import_table)
        db_table = options.import_table.split(".")
        if len(db_table) != 2:
            raise RuntimeError("Error: Invalid 'db.table' format: %s" % db_table)
        res["import_db_table"] = db_table

        # Parse fields
        if options.fields is None:
            res["fields"] = None
        else:
            res["fields"] = options.fields.split(",")

        if options.import_format == "csv":
            if options.delimiter is None:
                res["delimiter"] = ","
            else:
                if len(options.delimiter) == 1:
                    res["delimiter"] = options.delimiter
                elif options.delimiter == "\\t":
                    res["delimiter"] = "\t"
                else:
                    raise RuntimeError("Error: Must specify only one character for the --delimiter option")

            if options.custom_header is None:
                res["custom_header"] = None
            else:
                res["custom_header"] = options.custom_header.split(",")

            if options.no_header == True and options.custom_header is None:
                raise RuntimeError("Error: Cannot import a CSV file with --no-header and no --custom-header option")
            res["no_header"] = options.no_header
        else:
            if options.delimiter is not None:
                raise RuntimeError("Error: --delimiter option is only valid for CSV file formats")
            if options.no_header == True:
                raise RuntimeError("Error: --no-header option is only valid for CSV file formats")
            if options.custom_header is not None:
                raise RuntimeError("Error: --custom-header option is only valid for CSV file formats")

        res["primary_key"] = options.primary_key
    else:
        raise RuntimeError("Error: Must specify one of --directory or --file to import")

    return res

# This is run for each client requested, and accepts tasks from the reader processes
def client_process(host, port, auth_key, task_queue, error_queue, use_upsert):
    try:
        conn = r.connect(host, port, auth_key=auth_key)
        while True:
            task = task_queue.get()
            if len(task) == 3:
                # Unpickle objects (TODO: super inefficient, would be nice if we could pass down json)
                objs = [cPickle.loads(obj) for obj in task[2]]
                res = r.db(task[0]).table(task[1]).insert(objs, durability="soft", upsert=use_upsert).run(conn)
                if res["errors"] > 0:
                    raise RuntimeError("Error when importing into table '%s.%s': %s" %
                                       (task[0], task[1], res["first_error"]))
            else:
                break
    except (r.RqlError, r.RqlDriverError) as ex:
        error_queue.put((RuntimeError, RuntimeError(ex.message), traceback.extract_tb(sys.exc_info()[2])))
    except:
        ex_type, ex_class, tb = sys.exc_info()
        error_queue.put((ex_type, ex_class, traceback.extract_tb(tb)))

batch_length_limit = 200
batch_size_limit = 500000

class InterruptedError(Exception):
    def __str__(self):
        return "Interrupted"

# This function is called for each object read from a file by the reader processes
#  and will push tasks to the client processes on the task queue
def object_callback(obj, db, table, task_queue, object_buffers, buffer_sizes, fields, exit_event):
    global batch_size_limit
    global batch_length_limit

    if exit_event.is_set():
        raise InterruptedError()

    if not isinstance(obj, dict):
        raise RuntimeError("Error: Invalid input, expected an object, but got %s" % type(obj))

    # filter out fields
    if fields is not None:
        for key in list(obj.iterkeys()):
            if key not in fields:
                del obj[key]

    # Pickle the object here because we want an accurate size, and it'll pickle anyway for IPC
    object_buffers.append(cPickle.dumps(obj))
    buffer_sizes.append(len(object_buffers[-1]))
    if len(object_buffers) >= batch_length_limit or sum(buffer_sizes) > batch_size_limit:
        task_queue.put((db, table, object_buffers))
        del object_buffers[0:len(object_buffers)]
        del buffer_sizes[0:len(buffer_sizes)]
    return obj

json_read_chunk_size = 32 * 1024
json_max_buffer_size = 16 * 1024 * 1024

def read_json_single_object(json_data, file_in, callback):
    decoder = json.JSONDecoder()
    while True:
        try:
            (obj, offset) = decoder.raw_decode(json_data)
            json_data = json_data[offset:]
            callback(obj)
            break
        except ValueError:
            before_len = len(json_data)
            json_data += file_in.read(json_read_chunk_size)
            if before_len == len(json_data):
                raise
    return json_data

def read_json_array(json_data, file_in, callback, progress_info):
    decoder = json.JSONDecoder()
    file_offset = 0
    offset = 0
    while True:
        try:
            offset = json.decoder.WHITESPACE.match(json_data, offset).end()

            if json_data[offset] == "]": # End of JSON
                break

            (obj, offset) = decoder.raw_decode(json_data, idx=offset)
            callback(obj)

            # Read past whitespace to the next record
            file_offset += offset
            json_data = json_data[offset:]
            offset = json.decoder.WHITESPACE.match(json_data, 0).end()

            if json_data[offset] == ",":
                # Read past the comma
                offset = json.decoder.WHITESPACE.match(json_data, offset + 1).end()
            elif json_data[offset] != "]":
                raise ValueError("Error: JSON format not recognized - expected ',' or ']' after object")

        except (ValueError, IndexError):
            before_len = len(json_data)
            json_data += file_in.read(json_read_chunk_size)
            if json_data[offset] == ",":
                offset = json.decoder.WHITESPACE.match(json_data, offset + 1).end()
            elif before_len == len(json_data) or len(json_data) > json_max_buffer_size:
                raise
            progress_info[0].value = file_offset

    # Read the rest of the file and return it so it can be checked for unexpected data
    json_data += file_in.read()
    return json_data[offset + 1:]

def json_reader(task_queue, filename, db, table, primary_key, fields, progress_info, exit_event):
    object_buffers = []
    buffer_sizes = []

    with open(filename, "r") as file_in:
        # Scan to the first '[', then load objects one-by-one
        # Read in the data in chunks, since the json module would just read the whole thing at once
        json_data = file_in.read(json_read_chunk_size)

        callback = lambda x: object_callback(x, db, table, task_queue, object_buffers,
                                             buffer_sizes, fields, exit_event)

        progress_info[1].value = os.path.getsize(filename)

        offset = json.decoder.WHITESPACE.match(json_data, 0).end()
        if json_data[offset] == "[":
            json_data = read_json_array(json_data[offset + 1:], file_in, callback, progress_info)
        elif json_data[offset] == "{":
            json_data = read_json_single_object(json_data[offset:], file_in, callback)
        else:
            raise RuntimeError("Error: JSON format not recognized - file does not begin with an object or array")

        # Make sure only remaining data is whitespace
        while len(json_data) > 0:
            if json.decoder.WHITESPACE.match(json_data, 0).end() != len(json_data):
                raise RuntimeError("Error: JSON format not recognized - extra characters found after end of data")
            json_data = file_in.read(json_read_chunk_size)

    progress_info[0].value = progress_info[1].value

    if len(object_buffers) > 0:
        task_queue.put((db, table, object_buffers))

def csv_reader(task_queue, filename, db, table, primary_key, options, progress_info, exit_event):
    object_buffers = []
    buffer_sizes = []

    # Count the lines so we can report progress
    # TODO: this requires us to make two passes on csv files
    line_count = 0
    with open(filename, "r") as file_in:
        for i, l in enumerate(file_in):
            pass
        line_count = i + 1

    progress_info[1].value = line_count

    with open(filename, "r") as file_in:
        reader = csv.reader(file_in, delimiter=options["delimiter"])

        if not options["no_header"]:
            fields_in = reader.next()

        # Field names may override fields from the header
        if options["custom_header"] is not None:
            if not options["no_header"]:
                print "Ignoring header row: %s" % str(fields_in)
            fields_in = options["custom_header"]
        elif options["no_header"]:
            raise RuntimeError("Error: No field name information available")

        for row in reader:
            file_line = reader.line_num
            progress_info[0].value = file_line
            if len(fields_in) != len(row):
                raise RuntimeError("Error: File '%s' line %d has an inconsistent number of columns" % (filename, file_line))
            # We import all csv fields as strings (since we can't assume the type of the data)
            obj = dict(zip(fields_in, row))
            for key in list(obj.iterkeys()): # Treat empty fields as no entry rather than empty string
                if len(obj[key]) == 0:
                    del obj[key]
            object_callback(obj, db, table, task_queue, object_buffers, buffer_sizes, options["fields"], exit_event)

    if len(object_buffers) > 0:
        task_queue.put((db, table, object_buffers))

def table_reader(options, file_info, task_queue, error_queue, progress_info, exit_event):
    try:
        db = file_info["db"]
        table = file_info["table"]
        primary_key = file_info["info"]["primary_key"]
        conn = r.connect(options["host"], options["port"], auth_key=options["auth_key"])

        if table not in r.db(db).table_list().run(conn):
            r.db(db).table_create(table, primary_key=primary_key).run(conn)

        if file_info["format"] == "json":
            json_reader(task_queue,
                        file_info["file"],
                        db, table,
                        primary_key,
                        options["fields"],
                        progress_info,
                        exit_event)
        elif file_info["format"] == "csv":
            csv_reader(task_queue,
                       file_info["file"],
                       db, table,
                       primary_key,
                       options,
                       progress_info,
                       exit_event)
        else:
            raise RuntimeError("Error: Unknown file format specified")
    except (r.RqlError, r.RqlDriverError) as ex:
        error_queue.put((RuntimeError, RuntimeError(ex.message), traceback.extract_tb(sys.exc_info()[2])))
    except InterruptedError:
        pass # Don't save interrupted errors, they are side-effects
    except:
        ex_type, ex_class, tb = sys.exc_info()
        error_queue.put((ex_type, ex_class, traceback.extract_tb(tb), file_info["file"]))

def abort_import(signum, frame, parent_pid, exit_event, task_queue, clients, interrupt_event):
    # Only do the abort from the parent process
    if os.getpid() == parent_pid:
        interrupt_event.set()
        exit_event.set()

        alive_clients = sum([client.is_alive() for client in clients])
        for i in xrange(alive_clients):
            # TODO: this could theoretically block indefinitely if
            #   the queue is full and clients aren't reading
            task_queue.put("exit")

def print_progress(ratio):
    total_width = 40
    done_width = int(ratio * total_width)
    undone_width = total_width - done_width
    print "\r[%s%s] %3d%%" % ("=" * done_width, " " * undone_width, int(100 * ratio)),
    sys.stdout.flush()

def update_progress(progress_info):
    lowest_completion = 1.0
    for (current, max_count) in progress_info:
        curr_val = current.value
        max_val = max_count.value
        if curr_val < 0:
            lowest_completion = 0.0
        elif max_val <= 0:
            lowest_completion = 1.0
        else:
            lowest_completion = min(lowest_completion, float(curr_val) / max_val)

    print_progress(lowest_completion)

def spawn_import_clients(options, files_info):
    # Spawn one reader process for each db.table, as well as many client processes
    task_queue = multiprocessing.queues.SimpleQueue()
    error_queue = multiprocessing.queues.SimpleQueue()
    exit_event = multiprocessing.Event()
    interrupt_event = multiprocessing.Event()
    errors = []
    reader_procs = []
    client_procs = []

    parent_pid = os.getpid()
    signal.signal(signal.SIGINT, lambda a,b: abort_import(a, b, parent_pid, exit_event, task_queue, client_procs, interrupt_event))

    try:
        progress_info = [ ]

        for i in range(options["clients"]):
            client_procs.append(multiprocessing.Process(target=client_process,
                                                        args=(options["host"],
                                                              options["port"],
                                                              options["auth_key"],
                                                              task_queue,
                                                              error_queue,
                                                              options["force"])))
            client_procs[-1].start()

        for file_info in files_info:
            progress_info.append((multiprocessing.Value(ctypes.c_longlong, -1),
                                  multiprocessing.Value(ctypes.c_longlong, 0)))
            reader_procs.append(multiprocessing.Process(target=table_reader,
                                                        args=(options,
                                                              file_info,
                                                              task_queue,
                                                              error_queue,
                                                              progress_info[-1],
                                                              exit_event)))
            reader_procs[-1].start()

        # Wait for all reader processes to finish - hooray, polling
        while len(reader_procs) > 0:
            time.sleep(0.1)
            # If an error has occurred, exit out early
            if not error_queue.empty():
                exit_event.set()
            reader_procs = [proc for proc in reader_procs if proc.is_alive()]
            update_progress(progress_info)

        # Wait for all clients to finish
        alive_clients = sum([client.is_alive() for client in client_procs])
        for i in xrange(alive_clients):
            task_queue.put("exit")

        while len(client_procs) > 0:
            time.sleep(0.1)
            client_procs = [client for client in client_procs if client.is_alive()]

        # If we were successful, make sure 100% progress is reported
        if error_queue.empty() and not interrupt_event.is_set():
            print_progress(1.0)

        # Continue past the progress output line
        print ""
    finally:
        signal.signal(signal.SIGINT, signal.SIG_DFL)

    if interrupt_event.is_set():
        raise RuntimeError("Interrupted")

    if not task_queue.empty():
        error_queue.put((RuntimeError, RuntimeError("Error: Items remaining in the task queue"), None))

    if not error_queue.empty():
        # multiprocessing queues don't handling tracebacks, so they've already been stringified in the queue
        while not error_queue.empty():
            error = error_queue.get()
            print >> sys.stderr, "Traceback: %s" % (error[2])
            print >> sys.stderr, "%s: %s" % (error[0].__name__, error[1])
            if len(error) == 4:
                print >> sys.stderr, "In file: %s" % (error[3])
        raise RuntimeError("Errors occurred during import")

def get_import_info_for_file(filename, db_filter, table_filter):
    file_info = { }
    file_info["file"] = filename
    file_info["format"] = os.path.split(filename)[1].split(".")[-1]
    file_info["db"] = os.path.split(os.path.split(filename)[0])[1]
    file_info["table"] = os.path.split(filename)[1].split(".")[0]

    if len(db_filter) > 0 or len(table_filter) > 0:
        if file_info["db"] not in db_filter and (file_info["db"], file_info["table"]) not in table_filter:
            return None

    info_filepath = os.path.join(os.path.split(filename)[0], file_info["table"] + ".info")
    with open(info_filepath, "r") as info_file:
        file_info["info"] = json.load(info_file)

    return file_info

def import_directory(options):
    # Scan for all files, make sure no duplicated tables with different formats
    dbs = False
    db_filter = set([db_table[0] for db_table in options["tables"]]) | set(options["dbs"])
    files_to_import = []
    files_ignored = []
    for (root, dirs, files) in os.walk(options["directory"]):
        if not dbs:
            files_ignored.extend([os.path.join(root, f) for f in files])
            # The first iteration through should be the top-level directory, which contains the db folders
            dbs = True
            if len(db_filter) > 0:
                for i in range(len(dirs)):
                    if dirs[i] not in db_filter:
                        del dirs[i]
        else:
            if len(dirs) != 0:
                files_ignored.extend([os.path.join(root, d) for d in dirs])
                del dirs[0:len(dirs)]
            for f in files:
                split_file = f.split(".")
                if len(split_file) != 2 or split_file[1] not in ["json", "csv", "info"]:
                    files_ignored.append(os.path.join(root, f))
                elif split_file[1] == "info":
                    pass # Info files are included based on the data files
                elif not os.access(os.path.join(root, split_file[0] + ".info"), os.F_OK):
                    files_ignored.append(os.path.join(root, f))
                else:
                    files_to_import.append(os.path.join(root, f))

    # For each table to import collect: file, format, db, table, info
    files_info = []
    for filename in files_to_import:
        res = get_import_info_for_file(filename, options["dbs"], options["tables"])
        if res is not None:
            files_info.append(res)

    # Ensure no two files are for the same db/table, and that all formats are recognized
    db_tables = set()
    for file_info in files_info:
        if (file_info["db"], file_info["table"]) in db_tables:
            raise RuntimeError("Error: Duplicate db.table found in directory tree: %s.%s" % (file_info["db"], file_info["table"]))
        if file_info["format"] not in ["csv", "json"]:
            raise RuntimeError("Error: Unrecognized format for file %s" % file_info["file"])

        db_tables.add((file_info["db"], file_info["table"]))

    # Ensure that all needed databases exist and tables don't
    try:
        conn = r.connect(options["host"], options["port"], auth_key=options["auth_key"])
    except (r.RqlError, r.RqlDriverError) as ex:
        raise RuntimeError(ex.message)

    db_list = r.db_list().run(conn)
    for db in set([file_info["db"] for file_info in files_info]):
        if db not in db_list:
            r.db_create(db).run(conn)

    # Ensure that all tables do not exist (unless --forced)
    already_exist = []
    for file_info in files_info:
        table = file_info["table"]
        db = file_info["db"]
        if table in r.db(db).table_list().run(conn):
            if not options["force"]:
                already_exist.append("%s.%s" % (db, table))

            extant_primary_key = r.db(db).table(table).info().run(conn)["primary_key"]
            if file_info["info"]["primary_key"] != extant_primary_key:
                raise RuntimeError("Error: Table '%s.%s' already exists with a different primary key" % (db, table))

    if len(already_exist) == 1:
        raise RuntimeError("Error: Table '%s.%s' already exists, run with --force to import into the existing table" % (db, table))
    elif len(already_exist) > 1:
        already_exist.sort()
        extant_tables = "\n  ".join(already_exist)
        raise RuntimeError("Error: The following tables already exist, run with --force to import into the existing tables:\n  %s" % extant_tables)

    # Warn the user about the files that were ignored
    if len(files_ignored) > 0:
        print >> sys.stderr, "Unexpected files found in the specified directory.  Importing a directory expects"
        print >> sys.stderr, " a directory from `rethinkdb export`.  If you want to import individual tables"
        print >> sys.stderr, " import them as single files.  The following files were ignored:"
        for f in files_ignored:
            print >> sys.stderr, "%s" % str(f)

    spawn_import_clients(options, files_info)

def import_file(options):
    db = options["import_db_table"][0]
    table = options["import_db_table"][1]
    primary_key = options["primary_key"]

    try:
        conn = r.connect(options["host"], options["port"], auth_key=options["auth_key"])
    except (r.RqlError, r.RqlDriverError) as ex:
        raise RuntimeError(ex.message)

    # Ensure that the database and table exist
    if db not in r.db_list().run(conn):
        r.db_create(db).run(conn)

    if table in r.db(db).table_list().run(conn):
        if not options["force"]:
            raise RuntimeError("Error: Table already exists, run with --force if you want to import into the existing table")

        extant_primary_key = r.db(db).table(table).info().run(conn)["primary_key"]
        if primary_key is not None and primary_key != extant_primary_key:
            raise RuntimeError("Error: Table already exists with a different primary key")
        primary_key = extant_primary_key
    else:
        if primary_key is None:
            print "no primary key specified, using default primary key when creating table"
            r.db(db).table_create(table).run(conn)
        else:
            r.db(db).table_create(table, primary_key=primary_key).run(conn)

    # Make this up so we can use the same interface as with an import directory
    file_info = {}
    file_info["file"] = options["import_file"]
    file_info["format"] = options["import_format"]
    file_info["db"] = db
    file_info["table"] = table
    file_info["info"] = { "primary_key": primary_key }

    spawn_import_clients(options, [file_info])

def main():
    try:
        options = parse_options()
    except RuntimeError as ex:
        print >> sys.stderr, "Usage:\n%s" % usage
        print >> sys.stderr, ex
        return 1

    try:
        start_time = time.time()
        if "directory" in options:
            import_directory(options)
        elif "import_file" in options:
            import_file(options)
        else:
            raise RuntimeError("Error: Neither --directory or --file specified")
    except RuntimeError as ex:
        print >> sys.stderr, ex
        return 1
    print "  Done (%d seconds)" % (time.time() - start_time)
    return 0

if __name__ == "__main__":
    exit(main())
