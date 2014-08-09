#!/usr/bin/env python
import sys, os, datetime, time, shutil, tempfile, subprocess, os.path
from optparse import OptionParser
from ._backup import *

info = "'rethinkdb restore' loads data into a RethinkDB cluster from an archive"
usage = "rethinkdb restore FILE [-c HOST:PORT] [-a AUTH_KEY] [--clients NUM] [--force] [-i (DB | DB.TABLE)]..."

def print_restore_help():
    print info
    print usage
    print ""
    print "  FILE                             the archive file to restore data from"
    print "  -h [ --help ]                    print this help"
    print "  -c [ --connect ] HOST:PORT       host and client port of a rethinkdb node to connect"
    print "                                   to (defaults to localhost:28015)"
    print "  -a [ --auth ] AUTH_KEY           authorization key for rethinkdb clients"
    print "  -i [ --import ] (DB | DB.TABLE)  limit restore to the given database or table (may"
    print "                                   be specified multiple times)"
    print "  --clients NUM_CLIENTS            the number of client connections to use (defaults"
    print "                                   to 8)"
    print "  --temp-dir DIRECTORY             the directory to use for intermediary results"
    print "  --hard-durability                use hard durability writes (slower, but less memory"
    print "                                   consumption on the server)"
    print "  --force                          import data even if a table already exists"
    print ""
    print "EXAMPLES:"
    print ""
    print "rethinkdb restore rdb_dump.tar.gz -c mnemosyne:39500"
    print "  Import data into a cluster running on host 'mnemosyne' with a client port at 39500 using"
    print "  the named archive file."
    print ""
    print "rethinkdb restore rdb_dump.tar.gz -i test"
    print "  Import data into a local cluster from only the 'test' database in the named archive file."
    print ""
    print "rethinkdb restore rdb_dump.tar.gz -i test.subscribers -c hades -a hunter2"
    print "  Import data into a cluster running on host 'hades' which requires authorization from only"
    print "  a specific table from the named archive file."
    print ""
    print "rethinkdb restore rdb_dump.tar.gz --clients 4 --force"
    print "  Import data to a local cluster from the named archive file using only 4 client connections"
    print "  and overwriting any existing rows with the same primary key."

def parse_options():
    parser = OptionParser(add_help_option=False, usage=usage)
    parser.add_option("-c", "--connect", dest="host", metavar="HOST:PORT", default="localhost:28015", type="string")
    parser.add_option("-a", "--auth", dest="auth_key", metavar="KEY", default="", type="string")
    parser.add_option("-i", "--import", dest="tables", metavar="DB | DB.TABLE", default=[], action="append", type="string")

    parser.add_option("--temp-dir", dest="temp_dir", metavar="directory", default=None, type="string")
    parser.add_option("--clients", dest="clients", metavar="NUM_CLIENTS", default=8, type="int")
    parser.add_option("--hard-durability", dest="hard", action="store_true", default=False)
    parser.add_option("--force", dest="force", action="store_true", default=False)
    parser.add_option("--debug", dest="debug", default=False, action="store_true")
    parser.add_option("-h", "--help", dest="help", default=False, action="store_true")
    (options, args) = parser.parse_args()

    if options.help:
        print_restore_help()
        exit(0)

    # Check validity of arguments
    if len(args) == 0:
        raise RuntimeError("Error: Archive to import not specified.  Provide an archive file from rethinkdb-dump.")
    elif len(args) != 1:
        raise RuntimeError("Error: Only one positional argument supported")

    res = { }

    # Verify valid host:port --connect option
    (res["host"], res["port"]) = parse_connect_option(options.host)

    # Verify valid input file
    res["in_file"] = os.path.abspath(args[0])

    if not os.path.exists(res["in_file"]):
        raise RuntimeError("Error: Archive file does not exist: %s" % res["in_file"])

    # Verify valid --import options
    res["tables"] = parse_db_table_options(options.tables)

    # Make sure the temporary directory exists and is accessible
    res["temp_dir"] = options.temp_dir

    if res["temp_dir"] is not None:
        if not os.path.isdir(res["temp_dir"]):
            raise RuntimeError("Error: Temporary directory doesn't exist or is not a directory: %s" % res["temp_dir"])
        if not os.access(res["temp_dir"], os.W_OK):
            raise RuntimeError("Error: Temporary directory inaccessible: %s" % res["temp_dir"])

    res["auth_key"] = options.auth_key
    res["clients"] = options.clients
    res["hard"] = options.hard
    res["force"] = options.force
    res["debug"] = options.debug
    return res

def do_unzip(temp_dir, options):
    print "Unzipping archive file..."
    start_time = time.time()
    tar_args = ["tar", "xzf", options["in_file"], "--strip-components=1"]
    tar_args.extend(["-C", temp_dir])

    if sys.platform.startswith("linux"):
        # These are not supported on OSX (and not needed, default behavior works)
        tar_args.extend(["--force-local", "--wildcards"])

    # Only untar the selected tables
    # Apparently tar has problems if the same file is specified by two filters, so remove dupes
    dbs_to_export = set()
    for db, table in set(options["tables"]):
        if table is None:
            dbs_to_export.add(db)
            tar_args.append(os.path.join("*", db))
    for db, table in set(options["tables"]):
        if table is not None and db not in dbs_to_export:
            tar_args.append(os.path.join("*", db, table + ".*"))

    res = subprocess.call(tar_args)
    if res != 0:
        raise RuntimeError("Error: untar of archive '%s' failed" % options["in_file"])

    print "  Done (%d seconds)" % (time.time() - start_time)

def do_import(temp_dir, options):
    print "Importing from directory..."

    import_args = ["rethinkdb-import"]
    import_args.extend(["--connect", "%s:%s" % (options["host"], options["port"])])
    import_args.extend(["--directory", temp_dir])
    import_args.extend(["--auth", options["auth_key"]])
    import_args.extend(["--clients", str(options["clients"])])

    for db, table in options["tables"]:
        if table is None:
            import_args.extend(["--import", db])
        else:
            import_args.extend(["--import", "%s.%s" % (db, table)])

    if options["hard"]:
        import_args.append("--hard-durability")
    if options["force"]:
        import_args.append("--force")
    if options["debug"]:
        export_args.extend(["--debug"])

    res = subprocess.call(import_args)
    if res != 0:
        raise RuntimeError("Error: rethinkdb-import failed")

    # 'Done' message will be printed by the import script

def run_rethinkdb_import(options):
    # Create a temporary directory to store the extracted data
    temp_dir = tempfile.mkdtemp(dir=options["temp_dir"])
    res = -1

    try:
        do_unzip(temp_dir, options)
        do_import(temp_dir, options)
    except KeyboardInterrupt:
        time.sleep(0.2)
        raise RuntimeError("Interrupted")
    finally:
        shutil.rmtree(temp_dir)

def main():
    try:
        options = parse_options()
    except RuntimeError as ex:
        print >> sys.stderr, "Usage: %s" % usage
        print >> sys.stderr, ex
        return 1

    try:
        start_time = time.time()
        run_rethinkdb_import(options)
    except RuntimeError as ex:
        print >> sys.stderr, ex
        return 1
    return 0

if __name__ == "__main__":
    exit(main())
