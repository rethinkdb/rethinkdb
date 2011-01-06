#!/usr/bin/python
import sys, os, signal
from datetime import datetime
from subprocess import Popen, PIPE
from optparse import OptionParser
from cloud_retester import do_test, do_test_cloud, report_cloud, setup_testing_nodes, terminate_testing_nodes

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), 'common')))
from test_common import *
from git_util import *

long_test_branch = "ivan_long_test"
long_test_port = 10297
no_checkout_arg = "--no-checkout"
long_test_logs_dir = "~/long_test_logs"

rdb_num_threads = 12
rdb_db_files = ["/dev/sdb", "/dev/sdc", "/dev/sdd", "/dev/sde"] 
rdb_cache_size = 38000

rdbstat_path = "bench/dbench/rdbstat"

repo_dir = repo_toplevel()

def exec_self(args):
    os.execv("/usr/bin/env", ["python", sys.argv[0]] + args)

def git_checkout(branch):
    do_test("git fetch -f origin {b}:refs/remotes/origin/{b} && git checkout -f origin/{b}".format(b=branch))

def main():
    options = parse_arguments(sys.argv)

    if options['checkout']:
        if repo_has_local_changes():
            print >> sys.stderr, "Found local changes, bailing out"
            sys.exit(1)
        print "Pulling the latest updates to long-test branch"
        git_checkout(long_test_branch)
        exec_self(sys.argv[1:] + [no_checkout_arg])

    if options['make']:
        # Clean the repo
        do_test("cd ../src && make -j clean", cmd_format="make")

        # Build release build with symbols
        do_test("cd ../src && make -j DEBUG=0 SYMBOLS=1", cmd_format="make")

        # Make sure auxillary tools compile
        do_test("cd ../bench/stress-client/; make clean; make -j MYSQL=0 LIBMEMCACHED=0", cmd_format="make")

    ts = datetime.now().replace(microsecond=0)
    dir_name = "%s-%s" % (ts.isoformat(), repo_version())
    test_dir = TestDir(os.path.expanduser(os.path.join(long_test_logs_dir, dir_name)))
    print "Test directory: %s" % test_dir.name

    set_signal_handler()

    long_test_function(opts=options, test_dir=test_dir)

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

server = None
rdbstat = None
def long_test_function(opts, test_dir):
    global server
    global rdbstat
    print 'Starting server...'
    server = Server(opts, extra_flags=[], test_dir=test_dir)
    server.start()

    stats_file = test_dir.make_file("stats.gz")
    print "Collecting stats data into '%s'..." % stats_file
    rdbstat = RDBStatLogger(output=stats_file, port=server.port)
    rdbstat.start()

    print 'Running stress-client...'
    try:
        stress_client(port=server.port, workload =
                { "deletes":  opts["ndeletes"]
                , "updates":  opts["nupdates"]
                , "inserts":  opts["ninserts"]
                , "gets":     opts["nreads"]
                , "appends":  opts["nappends"]
                , "prepends": opts["nprepends"]
                }, duration=opts["duration"], test_dir = test_dir)
    except:
        shutdown()

def shutdown():
    global server
    global rdbstat
    if server:
        server.shutdown()   # this also stops stress_client
        server = None
    if rdbstat:
        rdbstat.stop()
        rdbstat = None

def set_signal_handler():
    def signal_handler(signum, frame):
        shutdown()

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGQUIT, signal_handler)



class RDBStatLogger(object):
    def __init__(self, output, host='localhost', port='11211', interval='1', count=None):
        self.output = file(output, "w")
        self.host = host
        self.port = port
        self.interval = interval
        self.count = count

    def start(self):
        rdbstat_args = [os.path.join(repo_dir, rdbstat_path), "--host", self.host, "--port", str(self.port), self.interval]
        if self.count:
            rdbstat_args.append(str(self.count))
        self.rdbstat = Popen(rdbstat_args, stdout=PIPE)
        self.gzip = Popen("gzip --rsyncable".split(' '), stdin=self.rdbstat.stdout, stdout=self.output)
     
    def stop(self):
        # Stop rdbstat, gzip will stop by itself when the pipe is closed and it's done writing
        terminate_process(self.rdbstat, wait_to_finish=False)

if __name__ == "__main__":
    main()
