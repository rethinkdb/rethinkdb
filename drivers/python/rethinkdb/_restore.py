#!/usr/bin/env python

'''`rethinkdb restore` loads data into a RethinkDB cluster from an archive'''

from __future__ import print_function

import copy, datetime, multiprocessing, optparse, os, shutil, sys, tarfile, tempfile, time, traceback
from . import net, utils_common, _import

usage = "rethinkdb restore FILE [-c HOST:PORT] [--tls-cert FILENAME] [-p] [--password-file FILENAME] [--clients NUM] [--shards NUM_SHARDS] [--replicas NUM_REPLICAS] [--force] [-i (DB | DB.TABLE)]..."
help_epilog = '''
FILE:
    the archive file to restore data from;
    if FILE is -, use standard input (note that
    intermediate files will still be written to
    the --temp-dir directory)

EXAMPLES:

rethinkdb restore rdb_dump.tar.gz -c mnemosyne:39500
  Import data into a cluster running on host 'mnemosyne' with a client port at 39500 using
  the named archive file.

rethinkdb restore rdb_dump.tar.gz -i test
  Import data into a local cluster from only the 'test' database in the named archive file.

rethinkdb restore rdb_dump.tar.gz -i test.subscribers -c hades -p
  Import data into a cluster running on host 'hades' which requires a password from only
  a specific table from the named archive file.

rethinkdb restore rdb_dump.tar.gz --clients 4 --force
  Import data to a local cluster from the named archive file using only 4 client connections
  and overwriting any existing rows with the same primary key.
'''

def parse_options(argv, prog=None):
    parser = utils_common.CommonOptionsParser(usage=usage, epilog=help_epilog, prog=prog)
    
    parser.add_option("-i", "--import",         dest="db_tables",  metavar="DB|DB.TABLE", default=[],     help="limit restore to the given database or table (may be specified multiple times)", action="append", type="db_table")

    parser.add_option("--temp-dir",             dest="temp_dir",   metavar="DIR",         default=None,   help="directory to use for intermediary results")
    parser.add_option("--clients",              dest="clients",    metavar="CLIENTS",     default=8,      help="client connections to use (default: 8)", type="pos_int")
    parser.add_option("--hard-durability",      dest="durability", action="store_const",  default="soft", help="use hard durability writes (slower, uses less memory)", const="hard")
    parser.add_option("--force",                dest="force",      action="store_true",   default=False,  help="import data even if a table already exists")
    parser.add_option("--no-secondary-indexes", dest="sindexes",   action="store_false",  default=True,   help="do not create secondary indexes for the restored tables")

    parser.add_option("--writers-per-table",    dest="writers",    metavar="WRITERS",     default=multiprocessing.cpu_count(), help=optparse.SUPPRESS_HELP, type="pos_int")
    parser.add_option("--batch-size",           dest="batch_size", metavar="BATCH",       default=_import.default_batch_size,  help=optparse.SUPPRESS_HELP, type="pos_int")
    
    # Replication settings
    replicationOptionsGroup = optparse.OptionGroup(parser, 'Replication Options')
    replicationOptionsGroup.add_option("--shards",   dest="create_args", metavar="SHARDS",   help="shards to setup on created tables (default: 1)",   type="pos_int", action="add_key")
    replicationOptionsGroup.add_option("--replicas", dest="create_args", metavar="REPLICAS", help="replicas to setup on created tables (default: 1)", type="pos_int", action="add_key")
    parser.add_option_group(replicationOptionsGroup)
        
    options, args = parser.parse_args(argv)

    # -- Check validity of arguments
    
    # - archive
    if len(args) == 0:
        parser.error("Archive to import not specified. Provide an archive file created by rethinkdb-dump.")
    elif len(args) != 1:
        parser.error("Only one positional argument supported")
    options.in_file = args[0]
    if options.in_file == '-':
        options.in_file = sys.stdin
    else:
        if not os.path.isfile(options.in_file):
            parser.error("Archive file does not exist: %s" % options.in_file)
        options.in_file = os.path.realpath(options.in_file)
    
    # - temp_dir
    if options.temp_dir:
        if not os.path.isdir(options.temp_dir):
            parser.error("Temporary directory doesn't exist or is not a directory: %s" % options.temp_dir)
        if not os.access(res["temp_dir"], os.W_OK):
            parser.error("Temporary directory inaccessible: %s" % options.temp_dir)
    
    # - create_args
    if options.create_args is None:
        options.create_args = {}
    
    # -- 
    
    return options

def do_unzip(temp_dir, options):
    '''extract the tarfile to the filesystem'''
    
    tables_to_export = set(options.db_tables)
    top_level        = None
    files_ignored    = []
    files_found      = False
    tarfileOptions   = {
        "mode": "r|*",
        "fileobj" if hasattr(options.in_file, "read") else "name": options.in_file 
    }
    with tarfile.open(**tarfileOptions) as archive:
        for tarinfo in archive:
            # skip without comment anything but files
            if not tarinfo.isfile():
                continue # skip everything but files
            
            # normalize the path
            relpath = os.path.relpath(os.path.realpath(tarinfo.name.strip().lstrip(os.sep)))
            
            # skip things that try to jump out of the folder
            if relpath.startswith(os.path.pardir):
                files_ignored.append(tarinfo.name)
                continue
            
            # skip files types other than what we use
            if not os.path.splitext(relpath)[1] in (".json", ".csv", ".info"):
                files_ignored.append(tarinfo.name)
                continue
            
            # ensure this looks like our structure
            try:
                top, db, file_name = relpath.split(os.sep)
            except ValueError:
                raise RuntimeError("Error: Archive file has an unexpected directory structure: %s" % tarinfo.name)
            
            if not top_level:
                top_level = top
            elif top != top_level:
                raise RuntimeError("Error: Archive file has an unexpected directory structure (%s vs %s)" % (top, top_level))
            
            # filter out tables we are not looking for
            table = os.path.splitext(file_name)
            if tables_to_export and not ((db, table) in tables_to_export or (db, None) in tables_to_export):
                continue # skip without comment
            
            # write the file out
            files_found = True
            dest_path = os.path.join(temp_dir, db, file_name)
            if not os.path.exists(os.path.dirname(dest_path)):
                os.makedirs(os.path.dirname(dest_path))
            with open(dest_path, 'wb') as dest:
                source = archive.extractfile(tarinfo)
                chunk = True
                while chunk:
                    chunk = source.read(1024 * 128)
                    dest.write(chunk)
                source.close()
            assert os.path.isfile(os.path.join(temp_dir, db, file_name))
    
    if not files_found:
        raise RuntimeError("Error: Archive file had no files")
    
    # - send the location and ignored list back to our caller
    return files_ignored

def do_restore(options):
    # Create a temporary directory to store the extracted data
    temp_dir = tempfile.mkdtemp(dir=options.temp_dir)

    try:
        # - extract the archive
        if not options.quiet:
            print("Extracting archive file...")
            start_time = time.time()
        
        files_ignored = do_unzip(temp_dir, options)
        
        if not options.quiet:
            print("  Done (%d seconds)" % (time.time() - start_time))
        
        # - default _import options
    
        options = copy.copy(options)
        options.fields = None
        options.directory = temp_dir
        
        # run the import
        if not options.quiet:
            print("Importing from directory...")
        
        try:
            _import.import_directory(options, files_ignored)
        except RuntimeError as ex:
            if options.debug:
                traceback.print_exc()
            if str(ex) == "Warnings occurred during import":
                raise RuntimeError("Warning: import did not create some secondary indexes.")
            else:
                errorString = str(ex)
                if errorString.startswith('Error: '):
                    errorString = errorString[len('Error: '):]
                raise RuntimeError("Error: import failed: %s" % errorString)
        # 'Done' message will be printed by the import script
    finally:
        shutil.rmtree(temp_dir)

def main(argv=None, prog=None):
    if argv is None:
        argv = sys.argv[1:]
    options = parse_options(argv, prog=prog)
    
    try:
        do_restore(options)
    except RuntimeError as ex:
        print(ex, file=sys.stderr)
        return 1
    return 0

if __name__ == "__main__":
    exit(main())
