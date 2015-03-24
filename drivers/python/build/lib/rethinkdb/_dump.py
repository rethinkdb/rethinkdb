#!/usr/bin/env python
from __future__ import print_function

import sys, os, datetime, time, shutil, tempfile, subprocess, os.path
from optparse import OptionParser
from ._backup import *

info = "'rethinkdb dump' creates an archive of data from a RethinkDB cluster"
usage = "rethinkdb dump [-c HOST:PORT] [-a AUTH_KEY] [-f FILE] [--clients NUM] [-e (DB | DB.TABLE)]..."

def print_dump_help():
    print(info)
    print(usage)
    print("")
    print("  -h [ --help ]                    print this help")
    print("  -c [ --connect ] HOST:PORT       host and client port of a rethinkdb node to connect")
    print("                                   to (defaults to localhost:28015)")
    print("  -a [ --auth ] AUTH_KEY           authorization key for rethinkdb clients")
    print("  -f [ --file ] FILE               file to write archive to (defaults to")
    print("                                   rethinkdb_dump_DATE_TIME.tar.gz)")
    print("  -e [ --export ] (DB | DB.TABLE)  limit dump to the given database or table (may")
    print("                                   be specified multiple times)")
    print("  --clients NUM_CLIENTS            number of tables to export simultaneously (defaults")
    print("                                   to 3)")
    print("  --temp-dir DIRECTORY             the directory to use for intermediary results")
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
    parser.add_option("-a", "--auth", dest="auth_key", metavar="key", default="", type="string")
    parser.add_option("-f", "--file", dest="out_file", metavar="file", default=None, type="string")
    parser.add_option("-e", "--export", dest="tables", metavar="(db | db.table)", default=[], action="append", type="string")

    parser.add_option("--temp-dir", dest="temp_dir", metavar="directory", default=None, type="string")
    parser.add_option("--clients", dest="clients", metavar="NUM", default=3, type="int")
    parser.add_option("--debug", dest="debug", default=False, action="store_true")
    parser.add_option("-h", "--help", dest="help", default=False, action="store_true")
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

    # Verify valid output file
    res["temp_filename"] = "rethinkdb_dump_%s" % datetime.datetime.today().strftime("%Y-%m-%dT%H:%M:%S")
    if options.out_file is None:
        res["out_file"] = os.path.abspath("./" + res["temp_filename"] + ".tar.gz")
    else:
        res["out_file"] = os.path.abspath(options.out_file)

    if os.path.exists(res["out_file"]):
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
    res["auth_key"] = options.auth_key
    res["debug"] = options.debug
    return res

def do_export(temp_dir, options):
    print("Exporting to directory...")
    export_args = ["rethinkdb-export"]
    export_args.extend(["--connect", "%s:%s" % (options["host"], options["port"])])
    export_args.extend(["--directory", os.path.join(temp_dir, options["temp_filename"])])
    export_args.extend(["--auth", options["auth_key"]])
    export_args.extend(["--clients", str(options["clients"])])

    for table in options["tables"]:
        export_args.extend(["--export", table])

    if options["debug"]:
        export_args.extend(["--debug"])

    res = subprocess.call(export_args)
    if res != 0:
        raise RuntimeError("Error: rethinkdb-export failed")

    # 'Done' message will be printed by the export script

def do_zip(temp_dir, options):
    print("Zipping export directory...")
    start_time = time.time()
    tar_args = ["tar", "czf", options["out_file"]]

    if sys.platform.startswith("linux"):
        # Tar on OSX does not support this flag, which may be useful with low free space
        tar_args.append("--remove-files")

    tar_args.extend(["-C", temp_dir])
    tar_args.append(options["temp_filename"])
    res = subprocess.call(tar_args)
    if res != 0:
        raise RuntimeError("Error: tar of export directory failed")
    print("  Done (%d seconds)" % (time.time() - start_time))

def run_rethinkdb_export(options):
    # Create a temporary directory to store the intermediary results
    temp_dir = tempfile.mkdtemp(dir=options["temp_dir"])
    res = -1

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
