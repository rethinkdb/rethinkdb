#!/usr/bin/python
import sys, os, signal, math
from datetime import datetime
from subprocess import Popen, PIPE
from optparse import OptionParser
from retester import send_email, do_test
import numpy as np
#import matplotlib.pyplot as plt
#import matplotlib.mlab as mlab

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
        exec_self([no_checkout_arg] + sys.argv[1:])

    if options['make']:
        # Clean the repo
        do_test("cd ../src && make -j clean", cmd_format="make")

        # Build release build with symbols
        do_test("cd ../src && make -j DEBUG=0 SYMBOLS=1 FAST_PERFMON=0", cmd_format="make")

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
    op['clients']   = IntFlag("--clients", 512)
    op['reporting_interval']   = IntFlag("--rinterval", 7200) # RSI
    op['emailfrom'] = StringFlag("--emailfrom", 'buildbot@rethinkdb.com:allspark')
    op['recipient'] = StringFlag("--email", 'ivan@rethinkdb.com') # RSI

    opts = op.parse(args)
    opts["netrecord"] = False   # We don't want to slow down the network
    opts['auto'] = True
    opts['mode'] = 'release'
    opts['valgrind'] = False

    return opts

server = None
rdbstat = None
stats_sender = None

def long_test_function(opts, test_dir):
    global server
    global rdbstat
    global stats_sender

    print 'Starting server...'
    server = Server(opts, extra_flags=[], test_dir=test_dir)
    server.start()

    stats_sender = StatsSender(opts, server)

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
                }, duration=opts["duration"], test_dir = test_dir,
                clients = opts["clients"])
    except:
        pass
    finally:
        shutdown()


def shutdown():
    global server
    global rdbstat
    global stats_sender
    if server:
        server.shutdown()   # this also stops stress_client
        server = None
    if rdbstat:
        rdbstat.stop()
        rdbstat = None
    if stats_sender:
        # stats_sender.post_results()
        stats_sender.stop()
        stats_sender = None

def set_signal_handler():
    def signal_handler(signum, frame):
        shutdown()

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGQUIT, signal_handler)

class StatsSender(object):
    monitoring = [
        'io_writes_started[iowrites]',
        'io_reads_started[ioreads]',
        'io_writes_completed[iowrites]',
        'io_reads_completed[ioreads]',

        'blocks_dirty[blocks]',
        'blocks_in_memory[blocks]',
        'cpu_user_avg',
        'cpu_system_avg',
        'cpu_combined_avg',
        'memory_faults_avg',
        'memory_real[bytes]',
        'memory_virtual[bytes]',
        'serializer_bytes_in_use',
        'serializer_old_garbage_blocks',
        'serializer_old_total_blocks',
        'serializer_reads_total',
        'serializer_writes_total',
        'uptime'
    ]
    def __init__(self, opts, server):
        def stats_aggregator(stats):
            stats_snapshot = {}
            for k in StatsSender.monitoring:
                if k in stats:
                    stats_snapshot[k] = stats[k]
            self.stats.append(stats_snapshot)

            if self.need_to_post():
                all_stats = self.reset_stats()
                self.post_results(all_stats) # new thread

        self.opts = opts
        self.stats = []

        self.rdbstat = RDBStat(('localhost', server.port), interval=1, stats_callback=stats_aggregator)
        self.rdbstat.start()

    def need_to_post(self):
        return self.stats[-1]['uptime'] - self.stats[0]['uptime'] >= self.opts['reporting_interval']

    def reset_stats(self):
        old_stats = self.stats
        self.stats = []
        return old_stats

    def post_results(self, all_stats):
        from email.mime.image import MIMEImage
        from email.mime.multipart import MIMEMultipart

        msg = MIMEMultipart()
        msg['Subject'] = 'Long test [%fs]' % all_stats[-1]['uptime']
        msg['From'] = 'buildbot@rethinkdb.com'
        msg['To'] = self.opts['recipient']

        msg.preamble = 'Preamble is here'

        for img in self.generate_plots(all_stats):
            msg.attach(MIMEImage(img))

        os.environ['RETESTER_EMAIL_SENDER'] = self.opts['emailfrom']
        send_email(None, msg, self.opts['recipient'])
        print "Sent an email"

    def generate_plots(self, all_stats):
        # tplot -o result.png -of png -if - -tf num -k 'cmd_get' 'quantilel 100 0.05,0.5,0.95' -k 'cmd_set' 'quantilel 100 0.05,0.5,0.95' -k transactions_starting_avg 'quantilel 100 0.05,0.5,0.95' -k cmd_set/s 'quantilel 100 0.05,0.5,0.95' -k cmd_get/s 'quantilel 100 0.05,0.5,0.95'  -k io_writes/s 'quantile 100 0.05,0.5,0.95' -k outstanding_writes 'quantile 100 0.05,0.5,0.95' -or 1024x2048
        plot_instructions = [ '-k', 'io_writes/s', 'quantile 36 0.05,0.5,0.95' ]
        tplot = Popen(['tplot', '-if', '-', '-o', '/dev/stdout', '-of', 'png', '-tf', 'num', '-or', '1024x768'] + plot_instructions, stdin=PIPE, stdout=PIPE)
        ts = None
        io_writes_total = None
        for stats in all_stats:
            old_ts = ts
            ts = stats['uptime']
            old_io_writes_total = io_writes_total
            io_writes_total = stats['io_writes_started[iowrites]']
            if old_ts and ts - old_ts > 0:
                print >>tplot.stdin, "%f =io_writes/s %f" % (ts, (io_writes_total - old_io_writes_total) / (ts - old_ts))
        tplot.stdin.close()
        return [tplot.stdout.read()]

    def stop(self):
        self.rdbstat.stop()


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
