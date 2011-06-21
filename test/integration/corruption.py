#!/usr/bin/python
""" 
    We want to make sure that the data remains valid as the server is killed and
    starts back up again. Ideally we would also like to validate the btree, but that should probably
    be done in C.
"""
import sys, os, time, random, threading
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

if __name__ == "__main__":

    op = make_option_parser()
    op["mclib"].default = "memcache"    # memcache plays nicer with this test than pylibmc does.
                                        # However, it causes connection hangups during verification for weird reasons.
                                        # We always use pylibmc during verification for that reason.
    op["extract"] = BoolFlag("--no-extract", invert = True)
    op["verify_timeout"] = IntFlag("--verify-timeout", 120) # Should be proportional to duration.
    op["missing_value_threshold"] = IntFlag("--missing-value-threshold", 0)
    opts = op.parse(sys.argv)

    test_dir = TestDir()
    mutual_dict = {}

    # Start first server

    opts_without_valgrind = dict(opts)
    opts_without_valgrind["valgrind"] = False
    insertion_server = Server(opts_without_valgrind, name="insertion server", extra_flags = ["--wait-for-flush", "y", "--flush-timer", "0"], test_dir = test_dir)
    insertion_server.start()
    server_killed = False

    # Run procedure to insert some values into database

    print "Inserting values..."
    def insert_values():
        mc = connect_to_port(opts, insertion_server.port)
        key = 0
        while not server_killed:
            key += 1
            value = str(random.randint(0, 9)) * random.randint(0, 10000)
            ok = mc.set(str(key), value)
            if ok == 0:
                raise ValueError("Could not insert %r" % value)
            mutual_dict[str(key)] = value
        mc.disconnect_all()   # At this point the server is dead anyway, but what the hell
    th = threading.Thread(target = insert_values)
    th.start()
    th.join(opts["duration"])

    # Make sure nothing went wrong in insertion of values

    if not th.is_alive():
        print "Insertion thread died prematurely; we will not go on to the next phase of the test."
        insertion_server.shutdown()
        sys.exit(1)

    # Kill first server

    server_killed = True
    time.sleep(1) # Let the thread finish inserting the value
    insertion_server.kill()

    # Store snapshot of possibly-corrupted data files

    print
    snapshot_dir = test_dir.p("db_data_snapshot")
    print "Storing snapshot of data files in %r." % snapshot_dir
    shutil.copytree(test_dir.p("db_data"), snapshot_dir)
    print

    # Start second server

    verification_server = Server(opts, name="verification server", data_files = insertion_server.data_files, test_dir = test_dir)
    verification_server.start()

    # Extract values to make sure the extractor is consistent with the server

    # TODO: Maybe the extraction test should be merged with this one completely (in that case this test should generate better values).
    if opts["extract"]:
        print "Extracting values..."
        dump_path = test_dir.p("db_data_dump.txt")
        run_executable(
            [get_executable_path(opts, "rethinkdb"), "extract", "-o", dump_path] + insertion_server.data_files.rethinkdb_flags(),
            "extractor_output.txt",
            valgrind_tool = opts["valgrind-tool"] if opts["valgrind"] else None,
            test_dir = test_dir,
            timeout = opts["verify_timeout"])

        dumpfile = open(dump_path, "r")
        dumplines = dumpfile.readlines()
        dumpfile.close()

        unchecked_keys = mutual_dict.keys()
        for i in xrange(0, len(dumplines) / 2):
            m = re.match(r"set (\d+) 0 0 (\d+) noreply\r\n", dumplines[2 * i])
            if m == None:
                raise ValueError("Invalid extraction for this test (line %d)" % (2 * i))
            (key, length) = m.groups()

            next_line = dumplines[2 * i + 1]
            value = next_line[0:int(length)]
            expected_crlf = next_line[int(length):]

            if value != mutual_dict[key]:
                raise ValueError("Invalid value for key '%s', has length %d, should have length %d (first character '%s', should be '%s')" % (key, len(value), len(mutual_dict[key]), value[0], mutual_dict[key][0]))
            if expected_crlf != "\r\n":
                raise ValueError("Lacking CRLF suffix for value of key '%s'" % key)
            unchecked_keys.remove(key)

        if len(unchecked_keys) != 0:
            raise ValueError("%d keys not found; one is '%s'" % (len(unchecked_keys), unchecked_keys[0]))

    # Check to make sure that the values are all present

    def verify_values(test_dir):
        # memcache seems to cause connection problems during verification. Switch over to pylibmc...
        original_mclib = opts["mclib"]
        opts["mclib"] = "pylibmc"
        mc = connect_to_port(opts, verification_server.port)
        successes = fails = 0
        for key in mutual_dict:
            value = mc.get(key)
            if mutual_dict[key] == value: successes += 1
            elif value is None: fails += 1
            else:
                # If the value was not merely missing, but actually turned into a different value,
                # then raise an error saying that
                raise ValueError("Expected %s, got %s" % (repr(value), repr(value2)))
        mc.disconnect_all()
        opts["mclib"] = original_mclib
        print "Out of %d values written, %d survived the server shutdown." % \
            (len(mutual_dict), successes)
        if fails > opts["missing_value_threshold"]:
            raise ValueError("Unacceptably many values were missing: %d" % fails)
    run_with_timeout(verify_values, timeout = opts["verify_timeout"], name = "verification test", test_dir = test_dir)

    # Clean up

    verification_server.shutdown()

