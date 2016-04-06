#!/usr/bin/env python
from __future__ import print_function

import sys, os, datetime, time, shutil, tarfile, tempfile, subprocess, os.path
from optparse import OptionParser
from ._backup import *

info = "'rethinkdb dump' creates an archive of data from a RethinkDB cluster"
usage = "rethinkdb dump [-c HOST:PORT] [-p] [--password-file FILENAME] [--tls-cert FILENAME] [-f FILE] [--clients NUM] [-e (DB | DB.TABLE)]..."

def print_dump_help():
    print(info)
    print(usage)
    print("")
    print("  -h [ --help ]                    print this help")
    print("  -c [ --connect ] HOST:PORT       host and client port of a rethinkdb node to connect")
    print("                                   to (defaults to localhost:28015)")
    print("  --tls-cert FILENAME              certificate file to use for TLS encryption.")
    print("  -p [ --password ]                interactively prompt for a password required to connect.")
    print("  --password-file FILENAME         read password required to connect from file.")
    print("  -f [ --file ] FILE               file to write archive to (defaults to")
    print("                                   rethinkdb_dump_DATE_TIME.tar.gz);")
    print("                                   if FILE is -, use standard output (note that")
    print("                                   intermediate files will still be written to")
    print("                                   the --temp-dir directory)")
    print("  -e [ --export ] (DB | DB.TABLE)  limit dump to the given database or table (may")
    print("                                   be specified multiple times)")
    print("  --clients NUM_CLIENTS            number of tables to export simultaneously (defaults")
    print("                                   to 3)")
    print("  --temp-dir DIRECTORY             the directory to use for intermediary results")
    print("  --overwrite-file                 don't abort when file given via --file already exists")
    print("  -q [ --quiet ]                   suppress non-error messages")
    print("")
    print("EXAMPLES:")
    print("rethinkdb dump -c mnemosyne:39500")
    print("  Archive all data from a cluster running on host 'mnemosyne' with a client port at 39500.")
    print("")
    print("rethinkdb dump -e test -f rdb_dump.tar.gz")
    print("  Archive only the 'test' database from a local cluster into a named file.")
    print("")
    print("rethinkdb dump -c hades -e test.subscribers -a hunter2")
    print("  Archive a specific table from a cluster running on host 'hades' which requires authorization.")

def parse_options():
    parser = OptionParser(add_help_option=False, usage=usage)
    parser.add_option("-c", "--connect", dest="host", metavar="host:port", default="localhost:28015", type="string")
    parser.add_option("-f", "--file", dest="out_file", metavar="file", default=None, type="string")
    parser.add_option("-e", "--export", dest="tables", metavar="(db | db.table)", default=[], action="append", type="string")

    parser.add_option("--tls-cert", dest="tls_cert", metavar="TLS_CERT", default="", type="string")

    parser.add_option("--temp-dir", dest="temp_dir", metavar="directory", default=None, type="string")
    parser.add_option("--overwrite-file", dest="overwrite_file", default=False, action="store_true")
    parser.add_option("--clients", dest="clients", metavar="NUM", default=3, type="int")
    parser.add_option("-q", "--quiet", dest="quiet", default=False, action="store_true")
    parser.add_option("--debug", dest="debug", default=False, action="store_true")
    parser.add_option("-h", "--help", dest="help", default=False, action="store_true")
    parser.add_option("-p", "--password", dest="password", default=False, action="store_true")
    parser.add_option("--password-file", dest="password_file", default=None, type="string")
    (options, args) = parser.parse_args()

    # Check validity of arguments
    if len(args) != 0:
        raise RuntimeError("Error: No positional arguments supported. Unrecognized option '%s'" % args[0])

    if options.help:
        print_dump_help()
        exit(0)

    res = {}

    # Verify valid host:port --connect option
    (res["host"], res["port"]) = parse_connect_option(options.host)

    res["tls_cert"] = options.tls_cert

    # Verify valid output file
    if sys.platform.startswith('win32') or sys.platform.startswith('cygwin'):
        res["temp_filename"] = "rethinkdb_dump_%s" % datetime.datetime.today().strftime("%Y-%m-%dT%H-%M-%S")
    else:
        res["temp_filename"] = "rethinkdb_dump_%s" % datetime.datetime.today().strftime("%Y-%m-%dT%H:%M:%S")

    if options.out_file == "-":
        res["out_file"] = sys.stdout
    else:
        # The output file is a real file in the file system
        if options.out_file is None:
            res["out_file"] = os.path.abspath("./" + res["temp_filename"] + ".tar.gz")
        else:
            res["out_file"] = os.path.abspath(options.out_file)

        if os.path.exists(res["out_file"]) and not options.overwrite_file:
            raise RuntimeError("Error: Output file already exists: %s" % res["out_file"])

    # Verify valid client count
    if options.clients < 1:
        raise RuntimeError("Error: invalid number of clients (%d), must be greater than zero" % options.clients)
    res["clients"] = options.clients

    # Make sure the temporary directory exists and is accessible
    res["temp_dir"] = options.temp_dir

    if res["temp_dir"] is not None:
        if not os.path.isdir(res["temp_dir"]):
            raise RuntimeError("Error: Temporary directory doesn't exist or is not a directory: %s" % res["temp_dir"])
        if not os.access(res["temp_dir"], os.W_OK):
            raise RuntimeError("Error: Temporary directory inaccessible: %s" % res["temp_dir"])

    res["tables"] = options.tables
    res["quiet"] = True if res["out_file"] is sys.stdout else options.quiet
    res["debug"] = options.debug
    res["password"] = options.password
    res["password-file"] = options.password_file
    return res

def do_export(temp_dir, options):
    if not options["quiet"]:
        print("Exporting to directory...")
    export_args = ["rethinkdb-export"]
    export_args.extend(["--connect", "%s:%s" % (options["host"], options["port"])])
    export_args.extend(["--directory", os.path.join(temp_dir, options["temp_filename"])])
    if options["password"]:
        export_args.append("--password")
    if options["password-file"]:
        export_args.extend(["--password-file", options["password-file"]])
    export_args.extend(["--clients", str(options["clients"])])
    export_args.extend(["--tls-cert", options["tls_cert"]])
    for table in options["tables"]:
        export_args.extend(["--export", table])

    if options["debug"]:
        export_args.extend(["--debug"])

    if options["quiet"]:
        export_args.extend(["--quiet"])

    res = subprocess.call(export_args)

    if res != 0:
        raise RuntimeError("Error: rethinkdb-export failed")

    # 'Done' message will be printed by the export script (unless options["quiet"])

def do_zip(temp_dir, options):
    if not options["quiet"]:
        print("Zipping export directory...")
    start_time = time.time()
    original_dir = os.getcwd()

    # Below,` tarfile.open()` forces us to set either `name` or `fileobj`,
    # depending on whether the output is a real file or an open file object.
    is_fileobj = type(options["out_file"]) is file
    name = None if is_fileobj else options["out_file"]
    fileobj = options["out_file"] if is_fileobj else None

    try:
        os.chdir(temp_dir)
        with tarfile.open(name=name, fileobj=fileobj, mode="w:gz") as f:
            for curr, subdirs, files in os.walk(options["temp_filename"]):
                for data_file in files:
                    path = os.path.join(curr, data_file)
                    f.add(path)
                    os.unlink(path)
    finally:
        os.chdir(original_dir)

    if not options["quiet"]:
        print("  Done (%d seconds)" % (time.time() - start_time))

def run_rethinkdb_export(options):
    # Create a temporary directory to store the intermediary results
    temp_dir = tempfile.mkdtemp(dir=options["temp_dir"])
    res = -1

    if not options["quiet"]:
        # Print a warning about the capabilities of dump, so no one is confused (hopefully)
        print("NOTE: 'rethinkdb-dump' saves data and secondary indexes, but does *not* save")
        print(" cluster metadata.  You will need to recreate your cluster setup yourself after ")
        print(" you run 'rethinkdb-restore'.")

    try:
        do_export(temp_dir, options)
        do_zip(temp_dir, options)
    except KeyboardInterrupt:
        time.sleep(0.2)
        raise RuntimeError("Interrupted")
    finally:
        shutil.rmtree(temp_dir)

def main():
    try:
        options = parse_options()
    except RuntimeError as ex:
        print("Usage: %s" % usage, file=sys.stderr)
        print(ex, file=sys.stderr)
        return 1

    try:
        start_time = time.time()
        run_rethinkdb_export(options)
    except RuntimeError as ex:
        print(ex, file=sys.stderr)
        return 1
    return 0

if __name__ == "__main__":
    exit(main())
