#!/usr/bin/python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import sys, os, signal, math, thread
from datetime import datetime
from subprocess import Popen, PIPE
from optparse import OptionParser
from retester import send_email, do_test
from threading import Timer

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), 'common')))
from test_common import *
from git_util import *
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), 'long_test')))
from statsDBCollector import *

long_test_branch = "long-test"
no_checkout_arg = "--no-checkout"
long_test_logs_dir = "~/long_test_logs"

rdb_num_threads = 12
rdb_db_files = ["/dev/sdb", "/dev/sdc", "/dev/sdd", "/dev/sde"]
rdb_cache_size = 25000

rdbstat_path = "bench/dbench/rdbstat"

repo_dir = repo_toplevel()

def exec_self(args):
    os.execv("/usr/bin/env", ["python", sys.argv[0]] + args)

def git_checkout(branch):
    do_test("git fetch -f origin {b}:refs/remotes/origin/{b} && git checkout -f origin/{b}".format(b=branch))

def main():
    try:
        options = parse_arguments(sys.argv)
    except OptError as e:
        print str(e)
        sys.exit(1)

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
        do_test("cd ../bench/stress-client/; make clean; make -j MYSQL=0 LIBMEMCACHED=0 stress libstress.so", cmd_format="make")

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
    op['ssds']      = AllArgsAfterFlag("--ssds")
    op["ndeletes"]  = IntFlag("--ndeletes",  1)
    op["nupdates"]  = IntFlag("--nupdates",  4)
    op["ninserts"]  = IntFlag("--ninserts",  8)
    op["nreads"]    = IntFlag("--nreads",    64)
    op["nappends"]  = IntFlag("--nappends",  0)
    op["nprepends"] = IntFlag("--nprepends", 0)
    op['duration']  = StringFlag("--duration", "infinity")
    op['checkout']  = BoolFlag(no_checkout_arg, invert = True)  # Tim says that means that checkout is True by default
    op['make']      = BoolFlag("--no-make", invert = True)
    op['clients']   = IntFlag("--clients", 512)
    op['slices']    = IntFlag("--slices", 36)
    op['reporting_interval']   = IntFlag("--rinterval", 28800)
    op['plot_width']   = IntFlag("--plot-width", 1024)
    op['single_plot_height']   = IntFlag("--single-plot-height", 144)
    op['emailfrom'] = StringFlag("--emailfrom", 'buildbot@rethinkdb.com:allspark')
    op['email'] = StringFlag("--email")
    op['db_server'] = StringFlag("--db-server", 'newton')
    op['db_user'] = StringFlag("--db-user", 'longtest')
    op['db_password'] = StringFlag("--db-password", 'rethinkdb2010')
    op['db_database'] = StringFlag("--db-database", 'longtest')
    op['test_id'] = StringFlag("--test-id", 'Long test')
    op['server_extra_flags'] = StringFlag("--server-extra", '')
    op['stress_extra_flags'] = StringFlag("--stress-extra", '')

    opts = op.parse(args)

    def split_words(s):
        if not(s):
            return []
        else:
            return s.split(' ')

    opts['server_extra_flags'] = split_words(opts['server_extra_flags'])
    opts['stress_extra_flags'] = split_words(opts['stress_extra_flags'])

    opts["netrecord"] = False   # We don't want to slow down the network
    opts['auto'] = True
    opts['mode'] = 'release'
    opts['valgrind'] = False

    return opts

server = None
rdbstat = None
stats_sender = None
stats_collector = None

def long_test_function(opts, test_dir):
    global server
    global rdbstat
    global stats_sender
    global stats_collector

    print 'Starting server...'
    server = Server(opts, extra_flags=opts['server_extra_flags'], test_dir=test_dir)
    server.start()

    stats_sender = StatsSender(opts, server)
    
    stats_collector = StatsDBCollector(opts, server)

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
                clients = opts["clients"], extra_flags=opts['stress_extra_flags'])
    finally:
        shutdown()


def shutdown():
    global server
    global rdbstat
    global stats_sender
    global stats_collector
    if stats_collector:
        stats_collector.stop()
        stats_collector = None

    errors = []
    if server:
        try:
            server.shutdown()   # this also stops stress_client
        except Exception as e:
            errors.append(e)
        server = None
    if rdbstat:
        try:
            rdbstat.stop()
        except Exception as e:
            errors.append(e)
        rdbstat = None
    if stats_sender:
        try:
            stats_sender.stop()
        except Exception as e:
            errors.append(e)
        stats_sender.post_results(errors=errors)
        stats_sender = None

def set_signal_handler():
    def signal_handler(signum, frame):
        shutdown()

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGQUIT, signal_handler)


from statsPlotter import *

# TODO: Use Plot from statsPlotter to generate individual plot images and put them in an HTML e-mail
class StatsSender(object):
    monitoring = [
        'active_coroutines',
        'blocks_dirty[blocks]',
        'blocks_in_memory[blocks]',
        'cmd_get_avg',
        'cmd_set_avg',
        'cmd_get_total',
        'cmd_set_total',
        'io_writes_started[iowrites]',
        'io_reads_started[ioreads]',
        'io_writes_completed[iowrites]',
        'io_reads_completed[ioreads]',
        'cpu_combined_avg',
        'cpu_system_avg',
        'cpu_user_avg',
        'memory_faults_max',
        'memory_real[bytes]',
        'memory_virtual[bytes]',
        'serializer_bytes_in_use',
        'serializer_old_garbage_blocks',
        'serializer_old_total_blocks',
        'serializer_reads_total',
        'serializer_writes_total',
        'serializer_lba_gcs',
        'serializer_data_extents_gced[dexts]',
        'transactions_starting_avg',
        'uptime',
        'timestamp'
    ]
    def __init__(self, opts, server):
        self.plot_width = int(opts['plot_width'])
        self.single_plot_height = int(opts['single_plot_height'])
        self.reporting_interval = int(opts['reporting_interval'])
        self.test_id = opts['test_id']
        desired_bucket_width = 4
        bucket_size = max(10, desired_bucket_width*float(self.reporting_interval)/float(self.plot_width))

        plot_style_quantile = 'quantile %d 0.05,0.5,0.95' % bucket_size
        plot_style_quantile_log = 'quantilel %d 0.05,0.5,0.95' % bucket_size
        plot_style_line = 'lines'

        self.plotters = [
            ('active_coroutines', plot_style_quantile, simple_plotter('active_coroutines')),
            ('blocks_dirty', plot_style_quantile, simple_plotter('blocks_dirty[blocks]')),
            ('blocks_in_memory', plot_style_line, simple_plotter('blocks_in_memory[blocks]')),
            ('cmd_get_avg', plot_style_quantile_log, simple_plotter('cmd_get_avg')),
            ('cmd_get/s', plot_style_quantile_log, differential_plotter('cmd_get_total')),
            ('cmd_set_avg', plot_style_quantile_log, simple_plotter('cmd_set_avg')),
            ('cmd_set/s', plot_style_quantile_log, differential_plotter('cmd_set_total')),
            ('io_writes/s', plot_style_quantile, differential_plotter('io_writes_started[iowrites]')),
            ('io_reads/s', plot_style_quantile, differential_plotter('io_reads_started[ioreads]')),
            ('outstanding_io_writes', plot_style_quantile, two_stats_diff_plotter('io_writes_started[iowrites]', 'io_writes_completed[iowrites]')),
            ('outstanding_io_reads', plot_style_quantile, two_stats_diff_plotter('io_reads_started[ioreads]', 'io_reads_completed[ioreads]')),
            ('cpu_combined_avg', plot_style_quantile, simple_plotter('cpu_combined_avg')),
            ('cpu_system_avg', plot_style_quantile, simple_plotter('cpu_system_avg')),
            ('cpu_user_avg', plot_style_quantile, simple_plotter('cpu_user_avg')),
            ('memory_faults_max', plot_style_quantile_log, simple_plotter('memory_faults_max')),
            ('memory_real', plot_style_line, simple_plotter('memory_real[bytes]', 1.0/(1024*1024))),
            ('memory_virtual', plot_style_line, simple_plotter('memory_virtual[bytes]', 1.0/(1024*1024))),
            ('gc_ratio', plot_style_line, two_stats_ratio_plotter('serializer_old_garbage_blocks', 'serializer_old_total_blocks')),
            ('serializer_MB_in_use', plot_style_line, simple_plotter('serializer_bytes_in_use', 1.0/(1024*1024))),
            ('serializer_reads/s', plot_style_quantile, differential_plotter('serializer_reads_total')),
            ('serializer_writes/s', plot_style_quantile, differential_plotter('serializer_writes_total')),
            ('serializer_data_blks_wr/s', plot_style_quantile, differential_plotter('serializer_writes_total')),
            ('gc_lba/s', plot_style_quantile, differential_plotter('serializer_lba_gcs')),
            ('gc_data_ext/s', plot_style_quantile, differential_plotter('serializer_data_extents_gced[dexts]')),
            ('txn_starting_avg', plot_style_quantile_log, simple_plotter('transactions_starting_avg')),
        ]
        def stats_aggregator(stats):
            stats_snapshot = {}
            for k in StatsSender.monitoring:
                if k in stats:
                    stats_snapshot[k] = stats[k]
            self.stats.append(stats_snapshot)

            if self.need_to_post():
                thread.start_new_thread(self.post_results, ())

        self.opts = opts
        self.stats = []

        self.rdbstat = RDBStat(('localhost', server.port), interval=1, stats_callback=stats_aggregator)
        self.rdbstat.start()

    def need_to_post(self):
        return self.stats[-1]['uptime'] - self.stats[0]['uptime'] >= self.reporting_interval

    def reset_stats(self):
        old_stats = self.stats
        self.stats = []
        return old_stats

    def post_results(self, errors=[]):
        all_stats = self.reset_stats()
        if len(all_stats) > 0:
            from email.mime.image import MIMEImage
            from email.mime.text import MIMEText
            from email.mime.multipart import MIMEMultipart

            def format_time(sec):
                days = int(sec/86400)
                sec = sec - days*86400
                hrs = int(sec/3600)
                sec = sec - hrs*3600
                mins = int(sec/60)
                sec = sec - mins*60
                if days > 0:
                    return '%d.%02d:%02d:%02d' % (days, hrs, mins, sec)
                else:
                    return '%02d:%02d:%02d' % (hrs, mins, sec)

            start_time = all_stats[0]['uptime']
            end_time = all_stats[-1]['uptime']

            msg = MIMEMultipart()
            msg['Subject'] = '[%s]: %s-%s' % (self.test_id, format_time(start_time), format_time(end_time))
            msg['From'] = 'buildbot@rethinkdb.com'
            msg['To'] = self.opts['email']

            img_number = 0
            images = self.generate_plots(all_stats)
            for img in images:
                msg_img = MIMEImage(img)
                msg_img.add_header('Content-ID', '<%d.png>' % img_number)
                msg_img.add_header('Content-Disposition', 'inline; filename=%s-%s-%d.png;' % (format_time(start_time), format_time(end_time), img_number))
                msg.attach(msg_img)

                img_number = img_number + 1

            html = """\
<html>
    <head>
    </head>
    <body>
        <img src="cid:0.png">
    </body>
</html>
"""
            msg.attach(MIMEText(html, 'html'))

            os.environ['RETESTER_EMAIL_SENDER'] = self.opts['emailfrom']
            send_email(None, msg, self.opts['email'])
            print "Sent an email"

    def generate_plots(self, all_stats):
        zipped_stats = zip(all_stats, all_stats[1:])
        plot_instructions = []
        for (name, plot_style, plotter) in self.plotters:
            plot_instructions.extend(['-k', name, plot_style])

        tplot = Popen(['tplot',
            '-if', '-',
            '-o', '/dev/stdout',
            '-of', 'png',
            '-tf', 'num',
            '-or', '%dx%d' % (self.plot_width, len(self.plotters)*self.single_plot_height),
            ] + plot_instructions, stdin=PIPE, stdout=PIPE)

        for stats_pair in zipped_stats:
            for (name, plot_style, plotter) in self.plotters:
                val = plotter(stats_pair)
                if val:
                    ts = stats_pair[1]['uptime']
                    print >>tplot.stdin, "%f =%s %f" % (ts, name, val)

        tplot.stdin.close()
        return [tplot.stdout.read()]

    def stop(self):
        self.rdbstat.stop()

def terminate_process(process, timeout=10, send_term=True, wait_to_finish=True):
    def send_kill():
        if process.poll() == None:
            process.kill()

    if send_term:
        process.terminate()

    if timeout:
        timer = Timer(timeout, send_kill)
        timer.start()

    if wait_to_finish:
        res = process.wait()
        timer.cancel()
        return res
    else:
        return None

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
