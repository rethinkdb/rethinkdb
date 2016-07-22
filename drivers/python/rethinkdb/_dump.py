#!/usr/bin/env python

'''`rethinkdb dump` creates an archive of data from a RethinkDB cluster'''

from __future__ import print_function

import datetime, os, platform, shutil, sys, tarfile, tempfile, time, traceback

from . import utils_common, net, _export
r = utils_common.r

usage = "rethinkdb dump [-c HOST:PORT] [-p] [--password-file FILENAME] [--tls-cert FILENAME] [-f FILE] [--clients NUM] [-e (DB | DB.TABLE)]..."
help_epilog = '''
EXAMPLES:
rethinkdb dump -c mnemosyne:39500
  Archive all data from a cluster running on host 'mnemosyne' with a client port at 39500.

rethinkdb dump -e test -f rdb_dump.tar.gz
  Archive only the 'test' database from a local cluster into a named file.

rethinkdb dump -c hades -e test.subscribers -p
  Archive a specific table from a cluster running on host 'hades' which requires a password.'''

def parse_options(argv, prog=None):
    parser = utils_common.CommonOptionsParser(usage=usage, epilog=help_epilog, prog=prog)
    
    parser.add_option("-f", "--file",     dest="out_file",  metavar="FILE",        default=None,  help='file to write archive to (defaults to rethinkdb_dump_DATE_TIME.tar.gz);\nif FILE is -, use standard output (note that intermediate files will still be written to the --temp-dir directory)')
    parser.add_option("-e", "--export",   dest="db_tables", metavar="DB|DB.TABLE", default=[],    help='limit dump to the given database or table (may be specified multiple times)', action="append")

    parser.add_option("--temp-dir",       dest="temp_dir",  metavar="directory",   default=None,   help='the directory to use for intermediary results')
    parser.add_option("--overwrite-file", dest="overwrite",                        default=False,  help="overwrite -f/--file if it exists", action="store_true")
    parser.add_option("--clients",        dest="clients",   metavar="NUM",         default=3,      help='number of tables to export simultaneously (default: 3)', type="pos_int")
    parser.add_option("--read-outdated",  dest="outdated",                         default=False,  help='use outdated read mode', action="store_true")
    
    options, args = parser.parse_args(argv)
    
    # Check validity of arguments
    
    if len(args) != 0:
        raise parser.error("No positional arguments supported. Unrecognized option(s): %s" % args)
    
    # Add dump name
    if platform.system() == "Windows" or platform.system().lower().startswith('cygwin'):
        options.dump_name = "rethinkdb_dump_%s" % datetime.datetime.today().strftime("%Y-%m-%dT%H-%M-%S") # no colons in name
    else:
        options.dump_name = "rethinkdb_dump_%s" % datetime.datetime.today().strftime("%Y-%m-%dT%H:%M:%S")
        
    # Verify valid output file
    if options.out_file == "-":
        options.out_file = sys.stdout
        options.quiet = True 
    elif options.out_file is None:
        options.out_file = os.path.realpath("%s.tar.gz" % options.dump_name)
    else:
        options.out_file = os.path.realpath(options.out_file)
    
    if not options.out_file is not sys.stdout:
        if os.path.exists(options.out_file) and not options.overwrite:
            parser.error("Output file already exists: %s" % options.out_file)
        if os.path.exists(options.out_file) and not os.path.isfile(options.out_file):
            parser.error("There is a non-file at the -f/--file location: %s" % options.out_file)

    # Verify valid client count
    if options.clients < 1:
        raise RuntimeError("Error: invalid number of clients (%d), must be greater than zero" % options.clients)

    # Make sure the temporary directory exists and is accessible
    if options.temp_dir is not None:
        if not exists(options.temp_dir):
            try:
                os.makedirs(options.temp_dir)
            except OSError:
                parser.error("Could not create temporary directory: %s" % options.temp_dir)
        if not os.path.isdir(options.temp_dir):
            parser.error("Temporary directory doesn't exist or is not a directory: %s" % options.temp_dir)
        if not os.access(options.temp_dir, os.W_OK):
            parser.error("Temporary directory inaccessible: %s" % options.temp_dir)
    
    return options

def run_rethinkdb_export(options):
    if not options.quiet:
        # Print a warning about the capabilities of dump, so no one is confused (hopefully)
        print("""\
NOTE: 'rethinkdb-dump' saves data and secondary indexes, but does *not* save
 cluster metadata.  You will need to recreate your cluster setup yourself after
 you run 'rethinkdb-restore'.""")
    
    try:
        start_time = time.time()
        
        # -- _export options - need to be kep in-sync with _export
        
        options.directory = os.path.realpath(tempfile.mkdtemp(dir=options.temp_dir))
        options.fields    = None
        options.delimiter = None
        options.format    = 'json'
        
        # -- export to a directory
        
        if not options.quiet:
            print("  Exporting to temporary directory...")
        
        try:
            _export.run(options)
        except Exception:
            if options.debug:
                sys.stderr.write('\n%s\n' % traceback.format_exc())
            raise Exception("Error: export failed")
        
        # -- zip directory
        
        if not options.quiet:
            print("  Zipping export directory...")
        
        try:
            archive = None
            if hasattr(options.out_file, 'read'):
                archive = tarfile.open(fileobj=options.out_file, mode="w:gz")
            else:
                archive = tarfile.open(name=options.out_file, mode="w:gz")
            
            for curr, _, files in os.walk(os.path.realpath(options.directory)):
                for data_file in files:
                    fullPath = os.path.join(options.directory, curr, data_file)
                    archivePath = os.path.join(options.dump_name, os.path.relpath(fullPath, options.directory))
                    archive.add(fullPath, arcname=archivePath)
                    os.unlink(fullPath)
        finally:
            if archive:
                archive.close()
        
        # --
        
        if not options.quiet:
            print("Done (%.2f seconds): %s" % (time.time() - start_time, options.out_file.name if hasattr(options.out_file, 'name') else options.out_file))
    except KeyboardInterrupt:
        time.sleep(0.2)
        raise RuntimeError("Interrupted")
    finally:
        if os.path.exists(options.directory):
            shutil.rmtree(options.directory)

def main(argv=None, prog=None):
    options = parse_options(argv or sys.argv[1:], prog=prog)

    try:
        run_rethinkdb_export(options)
    except Exception as ex:
        if options.debug:
            traceback.print_exc()
        print(ex, file=sys.stderr)
        return 1
    return 0

if __name__ == "__main__":
    sys.exit(main())
