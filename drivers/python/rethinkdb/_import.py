#!/usr/bin/env python

from __future__ import print_function

import codecs, csv, ctypes, datetime, json, multiprocessing, optparse
import os, re, signal, sys, time, traceback

from . import utils_common, net
r = utils_common.r

# Used because of API differences in the csv module, taken from
# http://python3porting.com/problems.html
PY3 = sys.version > '3'

#json parameters
json_read_chunk_size = 32 * 1024
json_max_buffer_size = 128 * 1024 * 1024
max_nesting_depth = 100
try:
    import cPickle as pickle
except ImportError:
    import pickle
try:
    from itertools import imap
except ImportError:
    imap = map
try:
    xrange
except NameError:
    xrange = range
try:
    from multiprocessing import SimpleQueue
except ImportError:
    from multiprocessing.queues import SimpleQueue

info = "'rethinkdb import` loads data into a RethinkDB cluster"
usage = "\
  rethinkdb import -d DIR [-c HOST:PORT] [--tls-cert FILENAME] [-p] [--password-file FILENAME]\n\
      [--force] [-i (DB | DB.TABLE)] [--clients NUM]\n\
      [--shards NUM_SHARDS] [--replicas NUM_REPLICAS]\n\
  rethinkdb import -f FILE --table DB.TABLE [-c HOST:PORT] [--tls-cert FILENAME] [-p] [--password-file FILENAME]\n\
      [--force] [--clients NUM] [--format (csv | json)] [--pkey PRIMARY_KEY]\n\
      [--shards NUM_SHARDS] [--replicas NUM_REPLICAS]\n\
      [--delimiter CHARACTER] [--custom-header FIELD,FIELD... [--no-header]]"

def print_import_help():
    print(info)
    print(usage)
    print("")
    print("  -h [ --help ]                    print this help")
    print("  -c [ --connect ] HOST:PORT       host and client port of a rethinkdb node to connect")
    print("                                   to (defaults to localhost:%d)" % net.DEFAULT_PORT)
    print("  --tls-cert FILENAME              certificate file to use for TLS encryption.")
    print("  -p [ --password ]                interactively prompt for a password required to connect.")
    print("  --password-file FILENAME         read password required to connect from file.")
    print("  --clients NUM_CLIENTS            the number of client connections to use (defaults")
    print("                                   to 8)")
    print("  --hard-durability                use hard durability writes (slower, but less memory")
    print("                                   consumption on the server)")
    print("  --force                          import data even if a table already exists, and")
    print("                                   overwrite duplicate primary keys")
    print("  --fields                         limit which fields to use when importing one table")
    print("  -q [ --quiet ]                   suppress non-error messages")
    print("")
    print("Import directory:")
    print("  -d [ --directory ] DIR           the directory to import data from")
    print("  -i [ --import ] (DB | DB.TABLE)  limit restore to the given database or table (may")
    print("                                   be specified multiple times)")
    print("  --no-secondary-indexes           do not create secondary indexes for the imported tables")
    print("")
    print("Import file:")
    print("  -f [ --file ] FILE               the file to import data from")
    print("  --table DB.TABLE                 the table to import the data into")
    print("  --format (csv | json)            the format of the file (defaults to json and accepts")
    print("                                   newline delimited json)")
    print("  --pkey PRIMARY_KEY               the field to use as the primary key in the table")
    print("")
    print("Import CSV format:")
    print("  --delimiter CHARACTER            character separating fields, or '\\t' for tab")
    print("  --no-header                      do not read in a header of field names")
    print("  --custom-header FIELD,FIELD...   header to use (overriding file header), must be")
    print("                                   specified if --no-header")
    print("")
    print("Import JSON format:")
    print("  --max-document-size              the maximum size in bytes that a single JSON document")
    print("                                   can have (defaults to 134217728).")
    print("  --max-nesting-depth              the maximum nesting depth of the JSON documents")
    print("")
    print("EXAMPLES:")
    print("")
    print("rethinkdb import -d rdb_export -c mnemosyne:39500 --clients 128")
    print("  Import data into a cluster running on host 'mnemosyne' with a client port at 39500,")
    print("  using 128 client connections and the named export directory.")
    print("")
    print("rethinkdb import -f site_history.csv --format csv --table test.history --pkey count")
    print("  Import data into a local cluster and the table 'history' in the 'test' database,")
    print("  using the named CSV file, and using the 'count' field as the primary key.")
    print("")
    print("rethinkdb import -d rdb_export -c hades -p -i test")
    print("  Import data into a cluster running on host 'hades' which requires a password,")
    print("  using only the database 'test' from the named export directory.")
    print("")
    print("rethinkdb import -f subscriber_info.json --fields id,name,hashtag --force")
    print("  Import data into a local cluster using the named JSON file, and only the fields")
    print("  'id', 'name', and 'hashtag', overwriting any existing rows with the same primary key.")
    print("")
    print("rethinkdb import -f user_data.csv --delimiter ';' --no-header --custom-header id,name,number")
    print("  Import data into a local cluster using the named CSV file with no header and instead")
    print("  use the fields 'id', 'name', and 'number', the delimiter is a semicolon (rather than")
    print("  a comma).")

def parse_options(argv):
    parser = optparse.OptionParser(add_help_option=False, usage=usage)
    parser.add_option("-c", "--connect", dest="host", metavar="HOST:PORT")
    parser.add_option("--fields", dest="fields", metavar="FIELD,FIELD...", default=None)
    parser.add_option("--clients", dest="clients", metavar="NUM_CLIENTS", default=8, type="int")
    parser.add_option("--hard-durability", dest="hard", action="store_true", default=False)
    parser.add_option("--force", dest="force", action="store_true", default=False)
    parser.add_option("-q", "--quiet", dest="quiet", default=False, action="store_true")
    parser.add_option("--debug", dest="debug", action="store_true", default=False)
    parser.add_option("--max-document-size", dest="max_document_size",  default=0,type="int")
    parser.add_option("--max-nesting-depth", dest="max_nesting_depth", default=0, type="int")

    # Replication settings
    parser.add_option("--shards", dest="shards", metavar="NUM_SHARDS", default=0, type="int")
    parser.add_option("--replicas", dest="replicas", metavar="NUM_REPLICAS", default=0, type="int")

    parser.add_option("--tls-cert", dest="tls_cert", metavar="TLS_CERT", default="")

    # Directory import options
    parser.add_option("-d", "--directory", dest="directory", metavar="DIRECTORY", default=None)
    parser.add_option("-i", "--import", dest="tables", metavar="DB | DB.TABLE", default=[], action="append")
    parser.add_option("--no-secondary-indexes", dest="create_sindexes", action="store_false", default=True)

    # File import options
    parser.add_option("-f", "--file", dest="import_file", metavar="FILE", default=None)
    parser.add_option("--format", dest="import_format", metavar="json | csv", default=None)
    parser.add_option("--table", dest="import_table", metavar="DB.TABLE", default=None)
    parser.add_option("--pkey", dest="primary_key", metavar="KEY", default=None)
    parser.add_option("--delimiter", dest="delimiter", metavar="CHARACTER", default=None)
    parser.add_option("--no-header", dest="no_header", action="store_true", default=False)
    parser.add_option("--custom-header", dest="custom_header", metavar="FIELD,FIELD...", default=None)
    parser.add_option("-h", "--help", dest="help", default=False, action="store_true")
    parser.add_option("-p", "--password", dest="password", default=False, action="store_true")
    parser.add_option("--password-file", dest="password_file", default=None)
    (options, args) = parser.parse_args(argv)

    # Check validity of arguments
    if len(args) != 0:
        raise RuntimeError("Error: No positional arguments supported. Unrecognized option '%s'" % args[0])

    if options.help:
        print_import_help()
        exit(0)

    res = {}

    # Verify valid host:port --connect option
    (res["host"], res["port"]) = utils_common.parse_connect_option(options.host)

    if options.clients < 1:
        raise RuntimeError("Error: --client option too low, must have at least one client connection")

    res["tls_cert"] = utils_common.ssl_option(options.tls_cert)
    res["clients"] = options.clients
    res["durability"] = "hard" if options.hard else "soft"
    res["force"] = options.force
    res["quiet"] = options.quiet
    res["debug"] = options.debug
    res["create_sindexes"] = options.create_sindexes

    res["create_args"] = { }
    for k in ['shards', 'replicas']:
        if getattr(options, k) != 0:
            res["create_args"][k] = getattr(options, k)

    # Default behavior for csv files - may be changed by options
    res["delimiter"] = ","
    res["no_header"] = False
    res["custom_header"] = None

    # buffer size
    if options.max_document_size > 0:
        global json_max_buffer_size
        json_max_buffer_size=options.max_document_size
    if options.max_nesting_depth > 0:
        global max_nesting_depth
        max_nesting_depth = options.max_nesting_depth
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
            raise RuntimeError("Error: Directory to import does not exist: %s" % res["directory"])

        # Verify valid --import options
        res["db_tables"] = utils_common.parse_db_table_options(options.tables)

        # Parse fields
        if options.fields is None:
            res["fields"] = None
        elif len(res["db_tables"]) != 1 or res["db_tables"][0][1] is None:
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
            options.import_format = os.path.split(options.import_file)[1].split(".")[-1]
            if options.import_format not in ["csv", "json"]:
                options.import_format = "json"

            res["import_format"] = options.import_format
        elif options.import_format not in ["csv", "json"]:
            raise RuntimeError("Error: Unknown format '%s', valid options are 'csv' and 'json'" % options.import_format)
        else:
            res["import_format"] = options.import_format

        # Verify valid --table option
        if options.import_table is None:
            raise RuntimeError("Error: Must specify a destination table to import into using the --table option")
        res["import_db_table"] = utils_common.parse_db_table(options.import_table)
        if res["import_db_table"][1] is None:
            raise RuntimeError("Error: Invalid 'db.table' format: %s" % options.import_table)

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

        if options.primary_key is not None:
          res["create_args"]["primary_key"] = options.primary_key
    else:
        raise RuntimeError("Error: Must specify one of --directory or --file to import")

    res["password"] = utils_common.get_password(options.password, options.password_file)
    return res

# This is called through utils_common.rdb_call_wrapper so reattempts can be tried as long as progress
# is being made, but connection errors occur.  We save a failed task in the progress object
# so it can be resumed later on a new connection.
def import_from_queue(progress, conn, task_queue, error_queue, replace_conflicts, durability, write_count):
    if progress[0] is not None and not replace_conflicts:
        # We were interrupted and it's not ok to overwrite rows, check that the batch either:
        # a) does not exist on the server
        # b) is exactly the same on the server
        task = progress[0]
        pkey = r.db(task[0]).table(task[1]).info().run(conn)["primary_key"]
        for i in reversed(range(len(task[2]))):
            obj = pickle.loads(task[2][i])
            if pkey not in obj:
                raise RuntimeError("Connection error while importing.  Current row has no specified primary key, so cannot guarantee absence of duplicates")
            row = r.db(task[0]).table(task[1]).get(obj[pkey]).run(conn)
            if row == obj:
                write_count[0] += 1
                del task[2][i]
            else:
                raise RuntimeError("Duplicate primary key `%s`:\n%s\n%s" % (pkey, str(obj), str(row)))

    task = task_queue.get() if progress[0] is None else progress[0]
    while not isinstance(task, StopIteration):
        try:
            # Unpickle objects (TODO: super inefficient, would be nice if we could pass down json)
            objs = [pickle.loads(obj) for obj in task[2]]
            conflict_action = 'replace' if replace_conflicts else 'error'
            res = r.db(task[0]).table(task[1]).insert(r.expr(objs, nesting_depth=max_nesting_depth), durability=durability, conflict=conflict_action).run(conn)
        except:
            progress[0] = task
            raise

        if res["errors"] > 0:
            raise RuntimeError("Error when importing into table '%s.%s': %s" %
                               (task[0], task[1], res["first_error"]))

        write_count[0] += len(objs)
        task = task_queue.get()

# This is run for each client requested, and accepts tasks from the reader processes
def client_process(host, port, task_queue, error_queue, rows_written, replace_conflicts, durability, ssl_op, admin_password):
    try:
        conn_fn = lambda: r.connect(host,
                                    port,
                                    ssl=ssl_op,
                                    user="admin",
                                    password=admin_password)
        write_count = [0]
        utils_common.rdb_call_wrapper(conn_fn, "import", import_from_queue, task_queue, error_queue, replace_conflicts, durability, write_count)
    except:
        ex_type, ex_class, tb = sys.exc_info()
        error_queue.put((ex_type, ex_class, traceback.extract_tb(tb)))

        # Read until the exit event so the readers do not hang on pushing onto the queue
        while not isinstance(task_queue.get(), StopIteration):
            pass

    with rows_written.get_lock():
        rows_written.value += write_count[0]

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
        for key in list(obj.keys()):
            if key not in fields:
                del obj[key]

    # Pickle the object here because we want an accurate size, and it'll pickle anyway for IPC
    object_buffers.append(pickle.dumps(obj))
    buffer_sizes.append(len(object_buffers[-1]))
    if len(object_buffers) >= batch_length_limit or sum(buffer_sizes) > batch_size_limit:
        task_queue.put((db, table, object_buffers))
        del object_buffers[0:len(object_buffers)]
        del buffer_sizes[0:len(buffer_sizes)]
    return obj


def read_json_array(json_data, file_in, callback, progress_info,
                    json_array=True):
    decoder = json.JSONDecoder()
    file_offset = 0
    offset = 0
    while True:
        try:
            offset = json.decoder.WHITESPACE.match(json_data, offset).end()
            if json_array and json_data[offset] == "]":
                break  # End of JSON
            (obj, offset) = decoder.raw_decode(json_data, idx=offset)
            callback(obj)

            # Read past whitespace to the next record
            file_offset += offset
            json_data = json_data[offset:]
            offset = json.decoder.WHITESPACE.match(json_data, 0).end()

            if json_array and json_data[offset] == ",":
                # Read past the comma
                offset = json.decoder.WHITESPACE.match(json_data, offset + 1).end()
            elif json_array and json_data[offset] != "]":
                raise ValueError("Error: JSON format not recognized - expected ',' or ']' after object")

        except (ValueError, IndexError):
            before_len = len(json_data)
            to_read = max(json_read_chunk_size, before_len)
            json_data += file_in.read(min(to_read, json_max_buffer_size - before_len))
            if json_array and json_data[offset] == ",":
                offset = json.decoder.WHITESPACE.match(json_data, offset + 1).end()
            elif (not json_array) and before_len == len(json_data):
                break  # End of JSON
            elif before_len == len(json_data) :
                raise
            elif len(json_data) >= json_max_buffer_size:
                raise ValueError("Error: JSON max buffer size exceeded. Use '--max-document-size' to extend your buffer.")
            progress_info[0].value = file_offset

    # Read the rest of the file and return it so it can be checked for unexpected data
    json_data += file_in.read()
    return json_data[offset + 1:]

def json_reader(task_queue, filename, db, table, fields, progress_info, exit_event):
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
        if json_data[offset] in "[{":
            json_data = read_json_array(
                json_data[offset + (1 if json_data[offset] == "[" else 0):],
                file_in, callback, progress_info,
                json_data[offset] == "[")
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

# Wrapper classes for the handling of unicode csv files
# Taken from https://docs.python.org/2/library/csv.html
class Utf8Recoder:
    def __init__(self, f):
        self.reader = codecs.getreader('utf-8')(f)

    def __iter__(self):
        return self

    def next(self):
        return self.reader.next().encode("utf-8")

class Utf8CsvReader:
    def __init__(self, f, **kwargs):
        f = Utf8Recoder(f)
        self.reader = csv.reader(f, **kwargs)
        self.line_num = self.reader.line_num

    def next(self):
        row = self.reader.next()
        self.line_num = self.reader.line_num
        return [unicode(s, 'utf-8') for s in row]

    def __iter__(self):
        return self

def open_csv_file(filename):
    if PY3:
        return open(filename, 'r', encoding='utf-8', newline='')
    else:
        return open(filename, 'r')

def csv_reader(task_queue, filename, db, table, options, progress_info, exit_event):
    object_buffers = []
    buffer_sizes = []

    # Count the lines so we can report progress
    # TODO: this requires us to make two passes on csv files
    line_count = 0
    with open_csv_file(filename) as file_in:
        for i, l in enumerate(file_in):
            pass
        line_count = i + 1

    progress_info[1].value = line_count

    with open_csv_file(filename) as file_in:
        if PY3:
            reader = csv.reader(file_in, delimiter=options["delimiter"])
        else:
            reader = Utf8CsvReader(file_in, delimiter=options["delimiter"])

        if not options["no_header"]:
            fields_in = next(reader)

        # Field names may override fields from the header
        if options["custom_header"] is not None:
            if not options["no_header"] and not options["quiet"]:
                print("Ignoring header row: %s" % str(fields_in))
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
            for key in list(obj.keys()): # Treat empty fields as no entry rather than empty string
                if len(obj[key]) == 0:
                    del obj[key]
            object_callback(obj, db, table, task_queue, object_buffers, buffer_sizes, options["fields"], exit_event)

    if len(object_buffers) > 0:
        task_queue.put((db, table, object_buffers))

# This function is called through utils_common.rdb_call_wrapper, which will reattempt if a connection
# error occurs.  Progress will resume where it left off.
def create_table(progress, conn, db, table, create_args, sindexes):
    # Make sure that the table is ready if it exists, or create it
    r.branch(r.db(db).table_list().contains(table),
        r.db(db).table(table).wait(timeout=30),
        r.db(db).table_create(table, **create_args)).run(conn)

    if progress[0] is None:
        progress[0] = 0

    # Recreate secondary indexes - assume that any indexes that already exist are wrong
    # and create them from scratch
    indexes = r.db(db).table(table).index_list().run(conn)
    created_indexes = list()
    try:
        for sindex in sindexes[progress[0]:]:
            if isinstance(sindex, dict) and all(k in sindex for k in ('index', 'function')):
                if sindex['index'] in indexes:
                    r.db(db).table(table).index_drop(sindex['index']).run(conn)
                r.db(db).table(table).index_create(sindex['index'], sindex['function']).run(conn)
                created_indexes.append(sindex['index'])
            progress[0] += 1
        r.db(db).table(table).index_wait(r.args(created_indexes)).run(conn)
    except RuntimeError:
        raise RuntimeError("Sindex warning")

def table_reader(options, file_info, task_queue, error_queue, warning_queue, progress_info, exit_event):
    try:
        db = file_info["db"]
        table = file_info["table"]
        create_args = dict(options["create_args"])
        create_args["primary_key"] = file_info["info"]["primary_key"]

        conn_fn = lambda: r.connect(options["host"],
                                    options["port"],
                                    ssl=options["tls_cert"],
                                    user="admin",
                                    password=options["password"])
        try:
            utils_common.rdb_call_wrapper(conn_fn, "create table", create_table, db, table, create_args,
                         file_info["info"]["indexes"] if options["create_sindexes"] else [])
        except RuntimeError as e:
            if str(e) == "Sindex warning":
                ex_type, ex_class, tb = sys.exc_info()
                warning_queue.put((ex_type, ex_class, traceback.extract_tb(tb), file_info["file"]))
            else:
                raise

        if file_info["format"] == "json":
            json_reader(task_queue,
                        file_info["file"],
                        db, table,
                        options["fields"],
                        progress_info,
                        exit_event)
        elif file_info["format"] == "csv":
            csv_reader(task_queue,
                       file_info["file"],
                       db, table,
                       options,
                       progress_info,
                       exit_event)
        else:
            raise RuntimeError("Error: Unknown file format specified")
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

def update_progress(progress_info, options):
    lowest_completion = 1.0
    for current, max_count in progress_info:
        curr_val = current.value
        max_val = max_count.value
        if curr_val < 0:
            lowest_completion = 0.0
        elif max_val <= 0:
            lowest_completion = 1.0
        else:
            lowest_completion = min(lowest_completion, float(curr_val) / max_val)

    if not options["quiet"]:
        utils_common.print_progress(lowest_completion)

def spawn_import_clients(options, files_info):
    # Spawn one reader process for each db.table, as well as many client processes
    task_queue = SimpleQueue()
    error_queue = SimpleQueue()
    warning_queue = SimpleQueue()
    exit_event = multiprocessing.Event()
    interrupt_event = multiprocessing.Event()
    errors = []
    reader_procs = []
    client_procs = []

    parent_pid = os.getpid()
    signal.signal(signal.SIGINT, lambda a, b: abort_import(a, b, parent_pid, exit_event, task_queue, client_procs, interrupt_event))

    try:
        progress_info = []
        rows_written = multiprocessing.Value(ctypes.c_longlong, 0)

        for i in xrange(options["clients"]):
            client_procs.append(multiprocessing.Process(target=client_process,
                                                        args=(options["host"],
                                                              options["port"],
                                                              task_queue,
                                                              error_queue,
                                                              rows_written,
                                                              options["force"],
                                                              options["durability"],
                                                              options["tls_cert"],
                                                              options["password"])))
            client_procs[-1].start()

        for file_info in files_info:
            progress_info.append((multiprocessing.Value(ctypes.c_longlong, -1), # Current lines/bytes processed
                                  multiprocessing.Value(ctypes.c_longlong, 0))) # Total lines/bytes to process
            reader_procs.append(multiprocessing.Process(target=table_reader,
                                                        args=(options,
                                                              file_info,
                                                              task_queue,
                                                              error_queue,
                                                              warning_queue,
                                                              progress_info[-1],
                                                              exit_event)))
            reader_procs[-1].start()

        # Wait for all reader processes to finish - hooray, polling
        while len(reader_procs) > 0:
            time.sleep(0.1)
            # If an error has occurred, exit out early
            while not error_queue.empty():
                exit_event.set()
                errors.append(error_queue.get())

            reader_procs = [proc for proc in reader_procs if proc.is_alive()]
            update_progress(progress_info, options)

        # Wait for all clients to finish
        alive_clients = sum([client.is_alive() for client in client_procs])
        for i in xrange(alive_clients):
            task_queue.put(StopIteration())

        while len(client_procs) > 0:
            time.sleep(0.1)
            client_procs = [client for client in client_procs if client.is_alive()]

        # If we were successful, make sure 100% progress is reported
        if len(errors) == 0 and not interrupt_event.is_set() and not options["quiet"]:
            utils_common.print_progress(1.0)

        def plural(num, text):
            return "%d %s%s" % (num, text, "" if num == 1 else "s")

        if not options["quiet"]:
            # Continue past the progress output line
            print("")
            print("%s imported in %s" % (plural(rows_written.value, "row"),
                                         plural(len(files_info), "table")))
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
            if len(error) == 4:
                print("In file: %s" % error[3], file=sys.stderr)
        raise RuntimeError("Errors occurred during import")

    if not warning_queue.empty():
        while not warning_queue.empty():
            warning = warning_queue.get()
            print("%s" % warning[1], file=sys.stderr)
            if options["debug"]:
                print("%s traceback: %s" % (warning[0].__name__, warning[2]), file=sys.stderr)
            if len(warning) == 4:
                print("In file: %s" % warning[3], file=sys.stderr)
        raise RuntimeError("Warnings occurred during import")

def get_import_info_for_file(filename, db_table_filter):
    file_info = {}
    file_info["file"] = filename
    file_info["format"] = os.path.split(filename)[1].split(".")[-1]
    file_info["db"] = os.path.split(os.path.split(filename)[0])[1]
    file_info["table"] = os.path.split(filename)[1].split(".")[0]

    if len(db_table_filter) > 0:
        if (file_info["db"], None) not in db_table_filter:
            if (file_info["db"], file_info["table"]) not in db_table_filter:
                return None

    info_filepath = os.path.join(os.path.split(filename)[0], file_info["table"] + ".info")
    with open(info_filepath, "r") as info_file:
        file_info["info"] = json.load(info_file)

    return file_info

def tables_check(progress, conn, files_info, force):
    # Ensure that all needed databases exist and tables don't
    db_list = r.db_list().run(conn)
    for db in set([file_info["db"] for file_info in files_info]):
        if db == "rethinkdb":
            raise RuntimeError("Error: Cannot import tables into the system database: 'rethinkdb'")
        if db not in db_list:
            r.db_create(db).run(conn)

    # Ensure that all tables do not exist (unless --forced)
    already_exist = []
    for file_info in files_info:
        table = file_info["table"]
        db = file_info["db"]
        if table in r.db(db).table_list().run(conn):
            if not force:
                already_exist.append("%s.%s" % (db, table))

            extant_pkey = r.db(db).table(table).info().run(conn)["primary_key"]
            if file_info["info"]["primary_key"] != extant_pkey:
                raise RuntimeError("Error: Table '%s.%s' already exists with a different primary key" % (db, table))

    return already_exist

def import_directory(options):
    # Scan for all files, make sure no duplicated tables with different formats
    dbs = False
    db_filter = set([db_table[0] for db_table in options["db_tables"]])
    files_to_import = []
    files_ignored = []
    for root, dirs, files in os.walk(options["directory"]):
        if not dbs:
            files_ignored.extend([os.path.join(root, f) for f in files])
            # The first iteration through should be the top-level directory, which contains the db folders
            dbs = True
            if len(db_filter) > 0:
                for i in reversed(xrange(len(dirs))):
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
        res = get_import_info_for_file(filename, options["db_tables"])
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

    conn_fn = lambda: r.connect(options["host"],
                                options["port"],
                                ssl=options["tls_cert"],
                                user="admin",
                                password=options["password"])
    # Make sure this isn't a pre-`reql_admin` cluster - which could result in data loss
    # if the user has a database named 'rethinkdb'
    utils_common.rdb_call_wrapper(conn_fn, "version check", utils_common.check_minimum_version, (1, 16, 0))
    already_exist = utils_common.rdb_call_wrapper(conn_fn, "tables check", tables_check, files_info, options["force"])

    if len(already_exist) == 1:
        raise RuntimeError("Error: Table '%s' already exists, run with --force to import into the existing table" % already_exist[0])
    elif len(already_exist) > 1:
        already_exist.sort()
        extant_tables = "\n  ".join(already_exist)
        raise RuntimeError("Error: The following tables already exist, run with --force to import into the existing tables:\n  %s" % extant_tables)

    # Warn the user about the files that were ignored
    if len(files_ignored) > 0:
        print("Unexpected files found in the specified directory.  Importing a directory expects", file=sys.stderr)
        print(" a directory from `rethinkdb export`.  If you want to import individual tables", file=sys.stderr)
        print(" import them as single files.  The following files were ignored:", file=sys.stderr)
        for f in files_ignored:
            print("%s" % str(f), file=sys.stderr)

    spawn_import_clients(options, files_info)

def table_check(progress, conn, db, table, create_args, force, quiet):
    pkey = None

    if db == "rethinkdb":
        raise RuntimeError("Error: Cannot import a table into the system database: 'rethinkdb'")

    if db not in r.db_list().run(conn):
        r.db_create(db).run(conn)

    if table in r.db(db).table_list().run(conn):
        if not force:
            raise RuntimeError("Error: Table already exists, run with --force if you want to import into the existing table")

        if 'primary_key' in create_args:
            pkey = r.db(db).table(table).info()["primary_key"].run(conn)
            if create_args["primary_key"] != pkey:
                raise RuntimeError("Error: Table already exists with a different primary key")
    else:
        if 'primary_key' in create_args:
            pkey = create_args["primary_key"]
        else:
            if not quiet:
                print("no primary key specified, using default primary key when creating table")
        r.db(db).table_create(table, **create_args).run(conn)

    return pkey

def import_file(options):
    db = options["import_db_table"][0]
    table = options["import_db_table"][1]

    # Ensure that the database and table exist with the right primary key
    conn_fn = lambda: r.connect(options["host"],
                                options["port"],
                                ssl=options["tls_cert"],
                                user="admin",
                                password=options["password"])
    # Make sure this isn't a pre-`reql_admin` cluster - which could result in data loss
    # if the user has a database named 'rethinkdb'
    utils_common.rdb_call_wrapper(conn_fn, "version check", utils_common.check_minimum_version, (1, 16, 0))
    pkey = utils_common.rdb_call_wrapper(conn_fn, "table check", table_check, db, table, options["create_args"], options["force"], options["quiet"])

    # Make this up so we can use the same interface as with an import directory
    file_info = {}
    file_info["file"] = options["import_file"]
    file_info["format"] = options["import_format"]
    file_info["db"] = db
    file_info["table"] = table
    file_info["info"] = {"primary_key": pkey, "indexes": []}

    spawn_import_clients(options, [file_info])

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
        start_time = time.time()
        if "directory" in options:
            import_directory(options)
        elif "import_file" in options:
            import_file(options)
        else:
            raise RuntimeError("Error: Neither --directory or --file specified")
    except RuntimeError as ex:
        print(ex, file=sys.stderr)
        if str(ex) == "Warnings occurred during import":
            return 2
        return 1
    if not options["quiet"]:
        print("  Done (%d seconds)" % (time.time() - start_time))
    return 0

if __name__ == "__main__":
    exit(main())
