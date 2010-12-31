#!/usr/bin/python
import sys
from subprocess import Popen, PIPE
import os
from optparse import OptionParser
from cloud_retester import do_test, do_test_cloud, report_cloud, setup_testing_nodes, terminate_testing_nodes

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), 'common')))
from test_common import *
from git_util import *

long_test_branch = "ivan_long_test"
long_test_port = 10297
no_checkout_arg = "--no-checkout"

rdb_num_threads = 12
rdb_db_files = ["/dev/sdb", "/dev/sdc", "/dev/sdd", "/dev/sde"] 
rdb_cache_size = 38000

def exec_self(args):
    os.execv("/usr/bin/env", ["python", sys.argv[0]] + args)

def git_checkout(branch):
    do_test("git fetch -f origin {b}:refs/remotes/origin/{b} && git checkout -f origin/{b}".format(b=branch))

def main():
    options = parse_arguments(sys.argv)

    if options['checkout']:
        if repo_has_local_changes():
            print "found local changes, bailing out"
            sys.exit(1)
        print "Pulling the latest updates to long-test branch"
        git_checkout(long_test_branch)
        exec_self(sys.argv[1:] + [no_checkout_arg])
    else:
        print "Checked out"

    if options['make']:
        # Clean the repo
        do_test("cd ../src && make -j clean", cmd_format="make")

        # Build release build with symbols
        do_test("cd ../src && make -j DEBUG=0 SYMBOLS=1", cmd_format="make")

        # Make sure auxillary tools compile
        do_test("cd ../bench/stress-client/; make clean; make -j MYSQL=0 LIBMEMCACHED=0", cmd_format="make")

    # Run the DB
    # build/release/rethinkdb serve -c 7 -m 40000 -f /dev/sdb -f /dev/sdc -f /dev/sdd -f /dev/sde 2>&1 | tee ../long_test_logs/server_log.txt
    auto_server_test_main(long_test_function, options, timeout=None)

    # Run stress-client
    #bench/stress-client/stress -s localhost:8080 -c 128 -w 0/0/1/0 -d infinity -o ../long_test_logs/keys

def parse_arguments(args):
    op = make_option_parser()
    op['cores']     = IntFlag("--cores",     rdb_num_threads)
    op['memory']    = IntFlag("--memory",    rdb_cache_size)
    op['ssds']      = AllArgsAfterFlag("--ssds", default = rdb_db_files)
    op["ndeletes"]  = IntFlag("--ndeletes",  1)
    op["nupdates"]  = IntFlag("--nupdates",  4)
    op["ninserts"]  = IntFlag("--ninserts",  8)
    op["nreads"]    = IntFlag("--nreads",    64)
    op["nappends"]  = IntFlag("--nappends",  0)
    op["nprepends"] = IntFlag("--nprepends", 0)
    op['duration']  = StringFlag("--duration", "30s") # RSI
    op['checkout']  = BoolFlag(no_checkout_arg, invert = True)  # Tim says that means that checkout is True by default
    op['make']      = BoolFlag("--no-make", invert = True)

    opts = op.parse(args)
    opts["netrecord"] = False   # We don't want to slow down the network
    opts['auto'] = True
    opts['mode'] = 'release'
    opts['valgrind'] = False

    return opts

def long_test_function(opts, port):
    print 'Running stress-client...'
    stress_client(port, workload = {
        "deletes":  opts["ndeletes"],
        "updates":  opts["nupdates"],
        "inserts":  opts["ninserts"],
        "gets":     opts["nreads"],
        "appends":  opts["nappends"],
        "prepends": opts["nprepends"]
        }, duration=opts["duration"])


if __name__ == "__main__":
    main()
