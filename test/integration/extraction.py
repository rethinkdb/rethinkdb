#!/usr/bin/python
""" 
    We want to make sure that the data remains valid as the server is killed and
    starts back up again. Ideally we would also like to validate the btree, but that should probably
    be done in C.
"""
import os, time, random, re
from test_common import *
import threading

if __name__ == "__main__":
    
    op = make_option_parser()
    op["mclib"].default = "memcache"   # memcache plays nicer with this test than pylibmc does
    opts = op.parse(sys.argv)
    
    make_test_dir()
    mutual_dict = {}
    
    # Start first server
    
    opts_without_valgrind = dict(opts)
    opts_without_valgrind["valgrind"] = False
    server1 = Server(opts_without_valgrind, "first server", extra_flags = ["--wait-for-flush", "y", "--flush-timer", "0"])
    if not server1.start(): sys.exit(1)
    
    # Run procedure to insert some values into database
    
    # print "Inserting values..."
    # def insert_values():
    #     mc = connect_to_port(opts, server1.port)
    #     for i in range(0, 100):
    #         value = str(random.randint(0, 10000))
    #         ok = mc.set(value, value)
    #         if ok == 0:
    #             raise ValueError("Could not insert %r" % value)
    #         mutual_list.append(value)
    #     mc.disconnect_all()
    # th = threading.Thread(target = insert_values)
    # th.start()
    # th.join(30)
    

    # Make sure nothing went wrong in insertion of values
    
    # if th.is_alive():
    #     print "Inserter thread didn't die; we will not go on to the next phase of the test."
    #     server1.shutdown()
    #     sys.exit(1)

    mc = connect_to_port(opts, server1.port)
    for i in xrange(0, 100):
        key = str(i)
        value = str(str(random.randint(0, 9)) * random.randint(0, 10000))
        ok = mc.set(key, value)
        if ok == 0:
            raise ValueError("Could not insert %r" % value)
        mutual_dict[key] = value
    mc.disconnect_all()

    
    # Kill first server
    
    if not server1.kill(): sys.exit(1)

    extract_path = get_executable_path(opts, "rethinkdb")
    dump_path = os.path.join(test_dir_name, "db_data_dump.txt")
    command_line = [extract_path, "extract",
                    "-f", os.path.join(test_dir_name, "db_data", "data_file"),
                    "-o", dump_path]

    subprocess.check_call(command_line)

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
            raise ValueError("Invalid value for key '%s'" % key)
        if expected_crlf != "\r\n":
            raise ValueError("Lacking CRLF suffix for value of key '%s'" % key)

    print "Extraction test succeeded."

# #!/usr/bin/python
# import subprocess
# from test_common import *

# if __name__ == "__main__":

#     op = make_option_parser()
#     op["mclib"].default = "memcache"    # memcache plays nicer with this test than pylibmc does
#     opts = op.parse(sys.argv)

#     make_test_dir()

#     # Start first server
    
#     opts_without_valgrind = dict(opts)
#     opts_without_valgrind["valgrind"] = False
#     server1 = Server(opts_without_valgrind, "first server", extra_flags = ["--wait-for-flush", "y", "--flush-timer", "0"])
#     if not server1.start(): sys.exit(1)
#     server_killed = False
    
#     # Run procedure to insert some values into database
    
#     print "Inserting values..."
#     def insert_values():
#         mc = connect_to_port(opts, server1.port)
#         for x in [("a", "12345"), ("b", "b" * 99999)]:
#             ok = mc.set(x[0], x[1])
#             if (ok == 0): raise ValueError("Could not insert value for key %s" % x[0])
#         mc.disconnect_all()
#     th = threading.Thread(target = insert_values)
#     th.start()
#     th.join(2)
    
#     # Make sure nothing went wrong in insertion of values
    
#     if not th.is_alive():
#         print "Inserter thread died prematurely; we will not go on to the next phase of the test."
#         server1.shutdown()
#         sys.exit(1)
    
#     # Kill first server
    
#     server_killed = True
#     if not server1.kill(): sys.exit(1)

#     extract_path = get_executable_path(opts, "rethinkdb")

#     dump_path = os.path.join(test_dir_name, "db_data_dump.txt")

#     command_line = [extract_path, "extract",
#                     "-f", os.path.join(test_dir_name, "db_data", "data_file"),
#                     "-o", dump_path]

#     subprocess.check_call(command_line)

#     dumpfile = open(dump_path, "r")
#     dumptxt = dumpfile.read()

#     lineA = "set a 0 0 5\r\n12345\r\n"
#     lineB = "set b 0 0 99999\r\n" + ("b" * 99999) + "\r\n"

#     if (dumptxt != lineA + lineB and dumptxt != lineB + lineA):
#         raise ValueError("Invalid extraction for this test.")

    



