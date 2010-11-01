#!/usr/bin/python
import subprocess
from test_common import *

if __name__ == "__main__":

    op = make_option_parser()
    opts = op.parse(sys.argv)

    make_test_dir()

    server = Server(opts)
    if not server.start(): sys.exit(1)
    mc = connect_to_server(opts, server)

    print "Testing extraction..."
    mc.set("a", "12345")
    mc.set("b", "b" * 99999);

    mc.disconnect_all()
    server.shutdown()

    extract_path = get_executable_path(opts, "rethinkdb-extract")

    dump_path = os.path.join(test_dir, "db_data_dump.txt")

    command_line = [extract_path,
                    os.path.join(test_dir, "db_data", "data_file"),
                    dump_path]

    subprocess.check_call(command_line)

    dumpfile = open(dump_path, "r")
    dumptxt = dumpfile.read()

    lineA = "set a 0 0 5\r\n12345\r\n"
    lineB = "set b 0 0 99999\r\n" + ("b" * 99999) + "\r\n"

    if (dumptxt != lineA + lineB and dumptxt != lineB + lineA):
        raise ValueError("Invalid extraction for this test.")

    



