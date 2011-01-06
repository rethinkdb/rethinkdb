#!/usr/bin/python
import sys, os, time, random, re, threading, struct
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
        value = str(str(random.randint(0, 9)) * random.randint(0, 10000))
        ok = mc.set(key, value)
        if ok == 0:
            raise ValueError("Could not insert %r" % value)
        mutual_dict[key] = value
    mc.disconnect_all()
    
    # Kill server
    
    server1.kill()

    npairs = None


    # Corrupt the file, slightly.  This is a bit of a hack because we
    # merrily assume the last leaf node we see in the file never
    # becomes an internal node.  This is unlikely but not impossible.
    # So we might get some clearly labeled RuntimeErrors.
    with open(server1.data_files.files[0], 'r+b') as f:
        done = False
        last_leaf_blockid = None
        while not done:
            block = f.read(4096)
            if block == '':
                done = True
            else:
                if block[12:16] == 'leaf':
                    last_leaf_blockid = block[0:4]

        if last_leaf_blockid == None:
            raise ValueError("the data file had no leaf nodes")

        f.seek(0)

        high_transaction_number = -1
        high_transaction_index = 0
        index = 0
        done = False
        while not done:
            block = f.read(4096)
            if block == '':
                done = True
            elif block[0:4] == last_leaf_blockid:
                trans_no = struct.unpack('=q', block[4:12])
                if trans_no > high_transaction_number:
                    high_transaction_number = trans_no
                    high_transaction_index = index
            index = index + 1

        if high_transaction_number == -1:
            raise RuntimeError("A block seems to have disappeared from the file!  This is very confusing.")

        f.seek(4096 * high_transaction_index)

        block = f.read(4096)
        if block[0:4] != last_leaf_blockid:
            raise RuntimeError("A block seems to have changed its block id!  This is very confusing.")

        if block[12:16] == 'inte':
            # We accidentally hit a leaf node that eventually becomes
            # an internal node.  This means the test broke, not
            # RethinkDB.  You could ignore this error, if it happens
            # rarely.  Maybe it's not worth fixing.
            raise RuntimeError("The block we are considering turned out to be an internal node (this is ok if it happens infrequently)")

        (npairs,) = struct.unpack('=H', block[24:26])

        # set pair_offsets[0] to 5000
        writeblock = block[0:28] + struct.pack('=H', 5000) + block[30:4096]
        f.seek(-4096, os.SEEK_CUR)
        f.write(writeblock)

    # Maybe we shouldn't modify the original file in place and instead write a copy.

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

    if npairs == None:
        raise ValueError("database file had no leaf nodes!??")

    if len(mutual_dict) - npairs < len(dumplines) / 2:
        raise ValueError("extraction dump has too many keys")
    elif len(mutual_dict) - npairs > len(dumplines) / 2:
        raise ValueError("extraction dump lacks keys")

    print "Extraction test succeeded."
