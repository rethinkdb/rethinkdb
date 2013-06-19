#!/usr/bin/env python
import sys, os, datetime, time, shutil, tempfile, subprocess
from optparse import OptionParser

def parse_options():
    parser = OptionParser()
    parser.add_option("-c", "--connect", dest="host", metavar="HOST:PORT", default="localhost:28015", type="string")
    parser.add_option("-a", "--auth", dest="auth_key", metavar="KEY", default="", type="string")
    parser.add_option("-o", dest="out_file", metavar="FILE", default=None, type="string")
    parser.add_option("-e", "--export", dest="tables", metavar="DB | DB.TABLE", default=[], action="append", type="string")
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

    # Verify valid output file
    res["temp_filename"] = "rethinkdb_export_%s" % datetime.datetime.today().strftime("%Y-%m-%dT%H:%M:%S")
    if options.out_file is None:
        res["out_file"] = os.path.abspath("./" + res["temp_filename"] + ".tar.gz")
    else:
        res["out_file"] = os.path.abspath(options.out_file)

    if os.path.exists(res["out_file"]):
        raise RuntimeError("Output file already exists")

    res["tables"] = options.tables
    res["auth_key"] = options.auth_key
    return res

def run_rethinkdb_export(options):
    # Find the export script
    export_script = os.path.abspath(os.path.join(os.path.dirname(__file__), "rethinkdb_export.py"))

    if not os.path.exists(export_script):
        raise RuntimeError("Could not find export script")

    # Create a temporary directory to store the intermediary results
    temp_dir = tempfile.mkdtemp()
    res = -1

    try:
        # TODO: filter stdout/stderr
        out_dir = os.path.join(temp_dir, options["temp_filename"])

        export_args = [export_script]
        export_args.extend(["--connect", "%s:%s" % (options["host"], options["port"])])
        export_args.extend(["--directory", out_dir])
        export_args.extend(["--auth", options["auth_key"]])
        for table in options["tables"]:
            export_args.extend(["--export", table])

        res = subprocess.call(export_args)
        if res != 0:
            raise RuntimeError("rethinkdb export failed")

        tar_args = ["tar", "czf", options["out_file"], "--remove-files"]
        tar_args.extend(["-C", temp_dir])
        tar_args.append(options["temp_filename"])
        res = subprocess.call(tar_args)
        if res != 0:
            raise RuntimeError("tar of export directory failed")
    finally:
        shutil.rmtree(temp_dir)

def main():
    try:
        options = parse_options()
        start_time = time.time()
        run_rethinkdb_export(options)
    except RuntimeError as ex:
        print ex
        return 1
    print "Done (%d seconds)" % (time.time() - start_time)
    return 0

if __name__ == "__main__":
    main()
