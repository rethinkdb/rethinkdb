#!/usr/bin/python
import sys, os, time, random, re, threading
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

if __name__ == "__main__":
    op = make_option_parser()
    op["mclib"].default = "memcache"   # memcache plays nicer with this test than pylibmc does
    opts = op.parse(sys.argv)
    
    test_dir = TestDir()
    mutual_dict = {}
    
    # Start server
    
    opts_without_valgrind = dict(opts)
    opts_without_valgrind["valgrind"] = False
    server1 = Server(opts_without_valgrind, name="first server", extra_flags = ["--wait-for-flush", "y", "--flush-timer", "0"], test_dir=test_dir)
    server1.start()

    # Put some values in server

    mc = connect_to_port(opts, server1.port)
    for i in xrange(0, 100):
        key = str(i)
        value = str(str(random.randint(0, 9)) * (random.randint(0, 10000) if i > 0 else 1001001))
        ok = mc.set(key, value)
        if ok == 0:
            raise ValueError("Could not insert %r" % value)
        mutual_dict[key] = value
    mc.disconnect_all()
    
    # Kill server
    
    server1.kill()

    # Run rethinkdb-extract

    extract_path = get_executable_path(opts, "rethinkdb")
    dump_path = test_dir.p("db_data_dump.txt")
    run_executable(
        [extract_path, "extract", "-o", dump_path] + server1.data_files.rethinkdb_flags(),
        "extractor_output.txt",
        valgrind_tool = opts["valgrind-tool"] if opts["valgrind"] else None,
        test_dir = test_dir
        )

    # Make sure extracted info is correct

    dumpfile = open(dump_path, "r")
    dumplines = dumpfile.readlines()

    equiv_dict = {}
    for i in xrange(0, len(dumplines) / 2):
        m = re.match(r"set (\d+) 0 0 (\d+) noreply\r\n", dumplines[2 * i])
        if m == None:
            raise ValueError("Invalid extraction for this test (line %d)" % (2 * i))
        (key, length) = m.groups()

        next_line = dumplines[2 * i + 1]
        value = next_line[0:int(length)]
        expected_crlf = next_line[int(length):]

        if value != mutual_dict[key]:
            #raise ValueError("Invalid value for key '%s', was '%s', should be '%s'" % (key, value, mutual_dict[key]))
            raise ValueError("Invalid value for key '%s', has length %d, should have length %d" % (key, len(value), len(mutual_dict[key])))
        if expected_crlf != "\r\n":
            raise ValueError("Lacking CRLF suffix for value of key '%s'" % key)

    print "Extraction test succeeded."
