#!/usr/bin/env python
import sys, os, datetime, time, threading, copy, json, traceback, csv
from optparse import OptionParser

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'drivers', 'python')))
import rethinkdb as r

def parse_options():
    parser = OptionParser()
    parser.add_option("-c", "--connect", dest="host", metavar="HOST:PORT", default="localhost:28015", type="string")
    parser.add_option("-a", "--auth", dest="auth_key", metavar="AUTHKEY", default="", type="string")
    parser.add_option("--fields", dest="fields", metavar="<FIELD>,<FIELD>...", default=None, type="string")
    parser.add_option("--force", dest="force", action="store_true", default=False)

    # Directory import options
    parser.add_option("-d", "--directory", dest="directory", metavar="DIRECTORY", default=None, type="string")
    parser.add_option("-i", "--import", dest="tables", metavar="DB | DB.TABLE", default=[], action="append", type="string")

    # File import options
    parser.add_option("-f", "--file", dest="import_file", metavar="FILE", default=None, type="string")
    parser.add_option("--format", dest="import_format", metavar="json | csv", default=None, type="string")
    parser.add_option("--table", dest="import_table", metavar="DB.TABLE", default=None, type="string")
    parser.add_option("--pkey", dest="primary_key", metavar="KEY", default = None, type="string")

    (options, args) = parser.parse_args()

    # Check validity of arguments
    if len(args) != 0:
        raise RuntimeError("No positional arguments supported")

    res = { }

    # Verify valid host:port --connect option
    host_port = options.host.split(":")
    if len(host_port) != 2:
        raise RuntimeError("Invalid 'host:port' format")
    (res["host"], res["port"]) = host_port

    res["auth_key"] = options.auth_key
    res["force"] = options.force

    if options.directory is not None:
        # Directory mode, verify directory import options
        if options.import_file is not None:
            raise RuntimeError("--file option is not valid when importing a directory")
        if options.import_format is not None:
            raise RuntimeError("--format option is not valid when importing a directory")
        if options.import_table is not None:
            raise RuntimeError("--table option is not valid when importing a directory")
        if options.primary_key is not None:
            raise RuntimeError("--pkey option is not valid when importing a directory")
            
        # Verify valid directory option
        dirname = options.directory
        res["directory"] = os.path.abspath(dirname)

        if not os.path.exists(res["directory"]):
            raise RuntimeError("directory to import does not exist")

        # Verify valid --import options
        res["dbs"] = []
        res["tables"] = []
        for item in options.tables:
            db_table = item.split(".")
            if len(db_table) == 1:
                res["dbs"].append(db_table[0])
            elif len(db_table) == 2:
                res["tables"].append(tuple(db_table))
            else:
                raise RuntimeError("Invalid 'db' or 'db.table' format: %s" % item)

        # Parse fields
        if options.fields is None:
            res["fields"] = None
        elif len(res["dbs"]) != 0 or len(res["tables"] != 1):
            raise RuntimeError("Can only use the --fields option when importing a single table")
        else:
            res["fields"] = options.fields.split(",")
        
    elif options.import_file is not None:
        # Single file mode, verify file import options
        if len(options.tables) != 0:
            raise RuntimeError("--import option is not valid when importing a single file")
        if options.directory is not None:
            raise RuntimeError("--directory option is not valid when importing a single file")

        import_file = options.import_file
        res["import_file"] = os.path.abspath(import_file)

        if not os.path.exists(res["import_file"]):
            raise RuntimeError("file to import does not exist")

        # Verify valid --format option
        if options.import_format is None:
            res["import_format"] = "json"
        elif options.import_format not in ["csv", "json"]:
            raise RuntimeError("Unknown format specified, valid options are 'csv' and 'json'")
        else:
            res["import_format"] = options.import_format
        
        # Verify valid --table option
        if options.import_table is None:
            raise RuntimeError("Must specify a destination table to import into using --table")
        db_table = options.import_table.split(".")
        if len(db_table) != 2:
            raise RuntimeError("Invalid 'db.table' format: %s" % db_table)
        res["import_db_table"] = db_table

        # Parse fields
        if options.fields is None:
            res["fields"] = None
        else:
            res["fields"] = options.fields.split(",")

        res["primary_key"] = options.primary_key
    else:
        raise RuntimeError("Must specify one of --directory or --file to import")

    return res

batch_length_limit = 200
batch_size_limit = 500000

def object_callback(obj, conn, db, table, object_buffers, buffer_sizes, fields):
    global batch_size_limit
    global batch_length_limit
    #TODO: filter out fields
    object_buffers.append(obj)
    buffer_sizes.append(sys.getsizeof(obj))
    if len(object_buffers) > batch_length_limit or sum(buffer_sizes) > batch_size_limit:
        r.db(db).table(table).insert(object_buffers).run(conn)
        del object_buffers[0:len(object_buffers)]
        del buffer_sizes[0:len(buffer_sizes)]
    return None

def restore_table_from_json(conn, filename, db, table, primary_key, fields):
    object_buffers = []
    buffer_sizes = []

    with open(filename, "r") as file_in:
        json.load(file_in, object_hook=lambda x: object_callback(x, conn, db, table, object_buffers, buffer_sizes, fields))

    if len(object_buffers) > 0:
        r.db(db).table(table).insert(object_buffers).run(conn)

def restore_table_from_csv(conn, filename, db, table, primary_key, fields):
    object_buffers = []
    buffer_sizes = []

    with open(filename, "r") as file_in:
        reader = csv.reader(file_in)
        # TODO: would be more efficient to skip fields that are filtered out here
        fields_in = reader.next()

        for row in reader:
            # TODO: probably shouldn't assume that each item in the csv is in json
            row = [json.loads(item) for item in row]
            obj = dict(zip(fields_in, row))
            object_callback(obj, conn, db, table, object_buffers, buffer_sizes, fields)

    if len(object_buffers) > 0:
        r.db(db).table(table).insert(object_buffers).run(conn)

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

# TODO: handle already-existing table case better - import into a different table name?
def import_table_client(options, file_info):
    db = file_info["db"]
    table = file_info["table"]
    primary_key = file_info["info"]["primary_key"]
    conn = r.connect(options["host"], options["port"], auth_key=options["auth_key"]);

    if table not in r.db(db).table_list().run(conn):
        r.db(db).table_create(table, primary_key=primary_key).run(conn)
    
    if file_info["format"] == "json":
        restore_table_from_json(conn, file_info["file"], db, table, primary_key, options["fields"])
    elif file_info["format"] == "csv":
        restore_table_from_csv(conn, file_info["file"], db, table, primary_key, options["fields"])
    else:
        raise RuntimeError("unknown file format specified")
    
def spawn_import_clients(options, files_info):
    # Spawn one client for each db.table
    # TODO: do this in a more efficient manner
    threads = []
    errors = []

    for file_info in files_info:
        args = {}
        args["options"] = options
        args["file_info"] = file_info
        threads.append(threading.Thread(target=import_table_client, kwargs=args))
        threads[-1].start()

    # Wait for all tables to finish
    for thread in threads:
        thread.join()
    
    if len(errors) != 0:
        print "%d errors occurred:" % len(errors)
        for error in errors:
            traceback.print_exception(error[0], error[1], error[2])

def import_directory(options):
    # Scan for all files, make sure no duplicated tables with different formats
    dbs = False
    db_filter = set([db_table[0] for db_table in options["tables"]]) | set(options["dbs"])
    files_to_import = []
    for (root, dirs, files) in os.walk(options["directory"]):
        if not dbs:
            # The first iteration through should be the top-level directory, which contains the db folders
            dbs = True
            if len(db_filter) > 0:
                for i in range(len(dirs)):
                    if dirs[i] not in db_filter:
                        del dirs[i]
        else:
            if len(dirs) != 0:
                raise RuntimeError("unexpected child directory in db folder: %s" % root)
            files_to_import.extend([os.path.join(root, f) for f in files if f.split(".")[-1] != "info"])

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
            raise RuntimeError("Duplicate db.table found in directory tree: %s.%s" % (file_info["db"], file_info["table"]))
        if file_info["format"] not in ["csv", "json"]:
            raise RuntimeError("Unrecognized format for file %s" % file_info["file"])

        db_tables.add((file_info["db"], file_info["table"]))

    # Ensure that all needed databases exist and tables don't
    conn = r.connect(options["host"], options["port"], auth_key=options["auth_key"])
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
                raise RuntimeError("table '%s.%s' already exists with a different primary key" % (db, table))

    if len(already_exist) == 1:
        raise RuntimeError("table '%s.%s' already exists, run with --force to import into the existing table" % (db, table))
    elif len(already_exist) > 1:
        already_exist.sort()
        extant_tables = "\n  ".join(already_exist)
        raise RuntimeError("the following tables already exist, run with --force to import into the existing tables:\n  %s" % extant_tables)

    spawn_import_clients(options, files_info)

def import_file(options):
    db = options["import_db_table"][0]
    table = options["import_db_table"][1]
    primary_key = options["primary_key"]
    conn = r.connect(options["host"], options["port"], auth_key=options["auth_key"]);

    # Ensure that the database and table exist
    if db not in r.db_list().run(conn):
        r.db_create(db).run(conn)

    if table in r.db(db).table_list().run(conn):
        if not options["force"]:
            raise RuntimeError("table already exists, run with --force if you want to import into the existing table")

        extant_primary_key = r.db(db).table(table).info().run(conn)["primary_key"]
        if primary_key is not None and primary_key != extant_primary_key:
            raise RuntimeError("table already exists with a different primary key")
        primary_key = extant_primary_key
    else:
        if primary_key is None:
            print "no primary key specified, using default primary key when creating table"
            r.db(db).table_create(table).run(conn)
        else:
            r.db(db).table_create(table, primary_key=primary_key).run(conn)
    
    if options["import_format"] == "json":
        restore_table_from_json(conn, options["import_file"], db, table, primary_key, options["fields"])
    elif options["import_format"] == "csv":
        restore_table_from_csv(conn, options["import_file"], db, table, primary_key, options["fields"])
    else:
        raise RuntimeError("unknown file format specified")

def main():
    try:
        options = parse_options()
        start_time = time.time()
        if "directory" in options:
            import_directory(options)
        elif "import_file" in options:
            import_file(options)
        else:
            raise RuntimeError("Neither --directory or --file specified")
    except RuntimeError as ex:
        print ex
        return 1
    print "Done (%d seconds)" % (time.time() - start_time)
    return 0

if __name__ == "__main__":
    main()
