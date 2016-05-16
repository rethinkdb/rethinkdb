#!/usr/bin/env python

from __future__ import print_function

import datetime, optparse, os, shutil, sys, tarfile, tempfile, time
from . import utils_common, net

info = "'rethinkdb restore' loads data into a RethinkDB cluster from an archive"
usage = "rethinkdb restore FILE [-c HOST:PORT] [--tls-cert FILENAME] [-p] [--password-file FILENAME] [--clients NUM] [--shards NUM_SHARDS] [--replicas NUM_REPLICAS] [--force] [-i (DB | DB.TABLE)]..."

def print_restore_help():
    print(info)
    print(usage)
    print("")
    print("  FILE                             the archive file to restore data from;")
    print("                                   if FILE is -, use standard input (note that")
    print("                                   intermediate files will still be written to")
    print("                                   the --temp-dir directory)")
    print("  -h [ --help ]                    print this help")
    print("  -c [ --connect ] HOST:PORT       host and client port of a rethinkdb node to connect")
    print("                                   to (defaults to localhost:%d)" % net.DEFAULT_PORT)
    print("  --tls-cert FILENAME              certificate file to use for TLS encryption.")
    print("  -p [ --password ]                interactively prompt for a password required to connect.")
    print("  --password-file FILENAME         read password required to connect from file.")
    print("  -i [ --import ] (DB | DB.TABLE)  limit restore to the given database or table (may")
    print("                                   be specified multiple times)")
    print("  --clients NUM_CLIENTS            the number of client connections to use (defaults")
    print("                                   to 8)")
    print("  --temp-dir DIRECTORY             the directory to use for intermediary results")
    print("  --hard-durability                use hard durability writes (slower, but less memory")
    print("                                   consumption on the server)")
    print("  --force                          import data even if a table already exists")
    print("  --no-secondary-indexes           do not create secondary indexes for the restored tables")
    print("  -q [ --quiet ]                   suppress non-error messages")
    print("")
    print("EXAMPLES:")
    print("")
    print("rethinkdb restore rdb_dump.tar.gz -c mnemosyne:39500")
    print("  Import data into a cluster running on host 'mnemosyne' with a client port at 39500 using")
    print("  the named archive file.")
    print("")
    print("rethinkdb restore rdb_dump.tar.gz -i test")
    print("  Import data into a local cluster from only the 'test' database in the named archive file.")
    print("")
    print("rethinkdb restore rdb_dump.tar.gz -i test.subscribers -c hades -p")
    print("  Import data into a cluster running on host 'hades' which requires a password from only")
    print("  a specific table from the named archive file.")
    print("")
    print("rethinkdb restore rdb_dump.tar.gz --clients 4 --force")
    print("  Import data to a local cluster from the named archive file using only 4 client connections")
    print("  and overwriting any existing rows with the same primary key.")

def parse_options(argv):
    parser = optparse.OptionParser(add_help_option=False, usage=usage)
    parser.add_option("-c", "--connect", dest="host", metavar="HOST:PORT")
    parser.add_option("-i", "--import", dest="tables", metavar="DB | DB.TABLE", default=[], action="append")

    parser.add_option("--shards", dest="shards", metavar="NUM_SHARDS", default=0, type="int")
    parser.add_option("--replicas", dest="replicas", metavar="NUM_REPLICAS", default=0, type="int")

    parser.add_option("--tls-cert", dest="tls_cert", metavar="TLS_CERT", default="")

    parser.add_option("--temp-dir", dest="temp_dir", metavar="directory", default=None)
    parser.add_option("--clients", dest="clients", metavar="NUM_CLIENTS", default=8, type="int")
    parser.add_option("--hard-durability", dest="hard", action="store_true", default=False)
    parser.add_option("--force", dest="force", action="store_true", default=False)
    parser.add_option("--no-secondary-indexes", dest="create_sindexes", action="store_false", default=True)
    parser.add_option("-q", "--quiet", dest="quiet", default=False, action="store_true")
    parser.add_option("--debug", dest="debug", default=False, action="store_true")
    parser.add_option("-h", "--help", dest="help", default=False, action="store_true")
    parser.add_option("-p", "--password", dest="password", default=False, action="store_true")
    parser.add_option("--password-file", dest="password_file", default=None)
    (options, args) = parser.parse_args(argv)

    if options.help:
        print_restore_help()
        exit(0)

    # Check validity of arguments
    if len(args) == 0:
        raise RuntimeError("Error: Archive to import not specified.  Provide an archive file from rethinkdb-dump.")
    elif len(args) != 1:
        raise RuntimeError("Error: Only one positional argument supported")

    res = {}

    # Verify valid host:port --connect option
    (res["host"], res["port"]) = utils_common.parse_connect_option(options.host)

    in_file_argument = args[0]

    if in_file_argument == "-":
        res["in_file"] = sys.stdin
    else:
        # Verify valid input file
        res["in_file"] = os.path.abspath(in_file_argument)

        if not os.path.exists(res["in_file"]):
            raise RuntimeError("Error: Archive file does not exist: %s" % res["in_file"])

    # Verify valid --import options
    res["tables"] = utils_common.parse_db_table_options(options.tables)

    # Make sure the temporary directory exists and is accessible
    res["temp_dir"] = options.temp_dir

    if res["temp_dir"] is not None:
        if not os.path.isdir(res["temp_dir"]):
            raise RuntimeError("Error: Temporary directory doesn't exist or is not a directory: %s" % res["temp_dir"])
        if not os.access(res["temp_dir"], os.W_OK):
            raise RuntimeError("Error: Temporary directory inaccessible: %s" % res["temp_dir"])

    res["clients"] = options.clients
    res["shards"] = options.shards
    res["replicas"] = options.replicas
    res["hard"] = options.hard
    res["force"] = options.force
    res["create_sindexes"] = options.create_sindexes
    res["quiet"] = options.quiet
    res["debug"] = options.debug

    res["tls_cert"] = options.tls_cert
    res["password"] = options.password
    res["password-file"] = options.password_file
    return res

def do_unzip(temp_dir, options):
    if not options["quiet"]:
        print("Unzipping archive file...")
    start_time = time.time()
    original_dir = os.getcwd()
    sub_path = None

    tables_to_export = set(options["tables"])
    members = []

    def parse_path(member):
        path = os.path.normpath(member.name)
        if member.isdir():
            return ('', '', '')
        if not member.isfile():
            raise RuntimeError("Error: Archive file contains an unexpected entry type")
        if not os.path.realpath(os.path.abspath(path)).startswith(os.getcwd()):
            raise RuntimeError("Error: Archive file contains unexpected absolute or relative path")
        path, table_file = os.path.split(path)
        path, db = os.path.split(path)
        path, base = os.path.split(path)
        return (base, db, os.path.splitext(table_file)[0])

    # If the in_file is a fileobj (e.g. stdin), stream it to a seekable file
    # first so that the code below can seek in it.
    tar_temp_file_path = None
    if options["in_file"] is sys.stdin:
        fileobj = options["in_file"]
        fd, tar_temp_file_path = tempfile.mkstemp(suffix=".tar.gz", dir=options["temp_dir"])
        # Constant memory streaming, buf == "" on EOF
        with os.fdopen(fd, "w", 65536) as f:
            buf = None
            while buf != "":
                buf = fileobj.read(65536)
                f.write(buf)
        name = tar_temp_file_path
    else:
        name = options["in_file"]

    try:
        with tarfile.open(name, "r:*") as f:
            os.chdir(temp_dir)
            try:
                for member in f:
                    base, db, table = parse_path(member)
                    if len(base) > 0:
                        if sub_path is None:
                            sub_path = base
                        elif sub_path != base:
                            raise RuntimeError("Error: Archive file has an unexpected directory structure (%s vs %s)" % (sub_path, base))

                        if len(tables_to_export) == 0 or \
                           (db, table) in tables_to_export or \
                           (db, None) in tables_to_export:
                            members.append(member)

                if sub_path is None:
                    raise RuntimeError("Error: Archive file had no files")

                f.extractall(members=members)

            finally:
                os.chdir(original_dir)
    finally:
        if tar_temp_file_path is not None:
            os.remove(tar_temp_file_path)

    if not options["quiet"]:
        print("  Done (%d seconds)" % (time.time() - start_time))
    return os.path.join(temp_dir, sub_path)

def do_import(temp_dir, options):
    from . import _import
    
    if not options["quiet"]:
        print("Importing from directory...")

    import_args = [
        "--connect", "%s:%s" % (options["host"], options["port"]),
        "--directory", temp_dir,
        "--clients", str(options["clients"]),
        "--shards", str(options["shards"]),
        "--replicas", str(options["replicas"]),
        "--tls-cert", options["tls_cert"]
    ]
    if options["password"]:
        import_args.append("--password")
    if options["password-file"]:
        import_args.extend(["--password-file", options["password-file"]])
    
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
        import_args.extend(["--debug"])
    if not options["create_sindexes"]:
        import_args.extend(["--no-secondary-indexes"])
    if options["quiet"]:
        import_args.extend(["--quiet"])
    
    res = _import.main(import_args)

    if res == 2:
        raise RuntimeError("Warning: import did not create some secondary indexes.")
    elif res != 0:
        raise RuntimeError("Error: import failed")
    # 'Done' message will be printed by the import script

def run_rethinkdb_import(options):
    # Create a temporary directory to store the extracted data
    temp_dir = tempfile.mkdtemp(dir=options["temp_dir"])
    res = -1

    try:
        sub_dir = do_unzip(temp_dir, options)
        do_import(sub_dir, options)
    except KeyboardInterrupt:
        time.sleep(0.2)
        raise RuntimeError("Interrupted")
    finally:
        shutil.rmtree(temp_dir)

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
        run_rethinkdb_import(options)
    except RuntimeError as ex:
        print(ex, file=sys.stderr)
        return 1
    return 0

if __name__ == "__main__":
    exit(main())
