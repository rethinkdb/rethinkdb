#!/usr/bin/env python

from __future__ import print_function

import datetime, optparse, os, shutil, sys, tarfile, tempfile, time

from . import utils_common, net
r = utils_common.r

info = ""
usage = "rethinkdb dump [-c HOST:PORT] [-p] [--password-file FILENAME] [--tls-cert FILENAME] [-f FILE] [--clients NUM] [-e (DB | DB.TABLE)]..."

def print_dump_help():
    print(''''rethinkdb dump' creates an archive of data from a RethinkDB cluster
%(usage)s
  -h [ --help ]                    print this help
  -c [ --connect ] HOST:PORT       host and client port of a rethinkdb node to connect
                                   to (defaults to localhost:%(default_port)d)
  --tls-cert FILENAME              certificate file to use for TLS encryption.
  -p [ --password ]                interactively prompt for a password required to connect.
  --password-file FILENAME         read password required to connect from file.
  -f [ --file ] FILE               file to write archive to (defaults to
                                   rethinkdb_dump_DATE_TIME.tar.gz);
                                   if FILE is -, use standard output (note that
                                   intermediate files will still be written to
                                   the --temp-dir directory)
  -e [ --export ] (DB | DB.TABLE)  limit dump to the given database or table (may
                                   be specified multiple times)
  --clients NUM_CLIENTS            number of tables to export simultaneously (default: 3)
  --temp-dir DIRECTORY             the directory to use for intermediary results
  --overwrite-file                 don't abort when file given via --file already exists
  -q [ --quiet ]                   suppress non-error messages

EXAMPLES:
rethinkdb dump -c mnemosyne:39500
  Archive all data from a cluster running on host 'mnemosyne' with a client port at 39500.

rethinkdb dump -e test -f rdb_dump.tar.gz
  Archive only the 'test' database from a local cluster into a named file.

rethinkdb dump -c hades -e test.subscribers -p
  Archive a specific table from a cluster running on host 'hades' which requires a password.
''' % {'usage':usage, 'default_port':net.DEFAULT_PORT})

def parse_options(argv):
    parser = optparse.OptionParser(add_help_option=False, usage=usage)
    parser.add_option("-c", "--connect", dest="host", metavar="host:port")
    parser.add_option("-f", "--file", dest="out_file", metavar="file", default=None)
    parser.add_option("-e", "--export", dest="tables", metavar="(db | db.table)", default=[], action="append")

    parser.add_option("--tls-cert", dest="tls_cert", metavar="TLS_CERT", default="")

    parser.add_option("--temp-dir", dest="temp_dir", metavar="directory", default=None)
    parser.add_option("--overwrite-file", dest="overwrite_file", default=False, action="store_true")
    parser.add_option("--clients", dest="clients", metavar="NUM", default=3, type="int")
    parser.add_option("-q", "--quiet", dest="quiet", default=False, action="store_true")
    parser.add_option("--debug", dest="debug", default=False, action="store_true")
    parser.add_option("-h", "--help", dest="help", default=False, action="store_true")
    parser.add_option("-p", "--password", dest="password", default=False, action="store_true")
    parser.add_option("--password-file", dest="password_file", default=None)
    
    options, args = parser.parse_args(argv)

    # Check validity of arguments
    if len(args) != 0:
        raise RuntimeError("Error: No positional arguments supported. Unrecognized option(s): %s" % args)

    if options.help:
        print_dump_help()
        exit(0)

    res = {}

    # Verify valid host:port --connect option
    (res["host"], res["port"]) = utils_common.parse_connect_option(options.host)

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
    from . import _export
    
    if not options["quiet"]:
        print("Exporting to directory...")
    export_args = [
        "--connect", "%s:%s" % (options["host"], options["port"]),
        "--directory", os.path.join(temp_dir, options["temp_filename"]),
        "--clients", str(options["clients"]),
        "--tls-cert", options["tls_cert"]
    ]
    if options["password"]:
        export_args.append("--password")
    if options["password-file"]:
        export_args.extend(["--password-file", options["password-file"]])
    for table in options["tables"]:
        export_args.extend(["--export", table])
    if options["debug"]:
        export_args.extend(["--debug"])
    if options["quiet"]:
        export_args.extend(["--quiet"])
    
    if _export.main(export_args) != 0:
        raise RuntimeError("Error: export failed")
    # 'Done' message will be printed by the export script (unless options["quiet"])

def do_zip(temp_dir, options):
    if not options["quiet"]:
        print("Zipping export directory...")
    start_time = time.time()

    # Below,` tarfile.open()` forces us to set either `name` or `fileobj`,
    # depending on whether the output is a real file or an open file object.
    try:
        archive = None
        if hasattr(options["out_file"], 'read'):
            archive = tarfile.open(fileobj=options["out_file"], mode="w:gz")
        else:
            archive = tarfile.open(name=options["out_file"], mode="w:gz")
        
        data_dir = os.path.join(temp_dir, options["temp_filename"])
        for curr, subdirs, files in os.walk(data_dir):
            for data_file in files:
                fullPath = os.path.join(data_dir, curr, data_file)
                archivePath = os.path.relpath(fullPath, temp_dir)
                archive.add(fullPath, arcname=archivePath)
                os.unlink(fullPath)
    finally:
        archive.close()

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
        run_rethinkdb_export(options)
    except RuntimeError as ex:
        print(ex, file=sys.stderr)
        return 1
    return 0

if __name__ == "__main__":
    exit(main())
