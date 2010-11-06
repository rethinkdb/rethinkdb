import sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir + '/oprofile')))
from plot import *
from oprofile import *
from profiles import *
import time
import StringIO
from line import *

class dbench():
    log_file = 'bench_log.txt'
    hostname = 'newton'
    www_dir = '/var/www/code.rethinkdb.com/htdocs/'
    prof_dir = 'prof_data' #directory on host where prof data goes
    out_dir = 'bench_html' #local directory to use for data
    bench_dir = 'bench_output'
    oprofile_dir = 'prof_output'
    flot_script_location = '/graph_viewer/index.html'

    def __init__(self, dir, email):
        self.email = email
        self.dir_str = time.asctime().replace(' ', '_').replace(':', '_')
        os.makedirs(self.out_dir + '/' + self.dir_str)
        self.bench_stats = self.bench_stats(dir + self.bench_dir)
        rundirs = []
        try:
            rundirs += os.listdir(dir + '/' + self.oprofile_dir)
            rundirs.remove(self.log_file)
            rundirs.sort(key = lambda x: int(x))
        except:
            print 'No OProfile data found'
        self.prof_stats = []
        for rundir in rundirs:
            self.prof_stats.append(self.oprofile_stats(dir + self.oprofile_dir + '/' + rundir + '/'))

    def report(self):
        self.html = self.report_as_html()
        self.push_html_to_host()
        self.send_email(self.email)
        os.system('rm -rf %s' % self.out_dir)

    class bench_stats():
        iostat_path     = 'iostat/output.txt'
        vmstat_path     = 'vmstat/output.txt'
        latency_path    = 'client/latency.txt'
        qps_path        = 'client/qps.txt'
        rdbstat_path    = 'rdbstat/output.txt'
        server_meta_path= 'server/output.txt'
        client_meta_path= 'client/output.txt'
        def __init__(self, dir):
            rundirs = []
            try:
                rundirs += os.listdir(dir)
            except:
                print 'No bench runs found'

            self.bench_runs = {}
            self.server_meta = {}
            self.client_meta = {}
            for rundir in rundirs:
                self.bench_runs[rundir] = [IOStat().read(dir + '/' + rundir + '/1/' + self.iostat_path),
                                             VMStat().read(dir + '/' + rundir + '/1/' + self.vmstat_path),
                                             Latency().read(dir + '/' + rundir + '/1/' + self.latency_path),
                                             QPS().read(dir + '/' + rundir + '/1/' + self.qps_path),
                                             RDBStats().read(dir + '/' + rundir + '/1/' + self.rdbstat_path)]
                try:
                   self.server_meta[rundir] = (open(dir + '/' + rundir + '/1/' + self.server_meta_path).read())
                except IOError:
                    self.server_meta[rundir] = ''
                    print "No meta data for server found"

                try:
                   self.client_meta[rundir] = (open(dir + '/' + rundir + '/1/' + self.client_meta_path).read())
                except IOError:
                    self.client_meta[rundir] = ''
                    print "No meta data for client found"

        def parse_server_meta(self, data):
            threads_line = line('Number of DB threads: (\d+)', [('threads', 'd')])
            m = until(threads_line, data)
            assert m != False
            return "Threads: %d" % m['threads']

        def parse_client_meta(self, data):
            client_line = line('\[host: [\d\.]+, port: \d+, clients: \d+, load: (\d+)/(\d+)/(\d+)/(\d+), keys: \d+-\d+, values: \d+-\d+ , duration: (\d+), batch factor: \d+-\d+, latency file: latency.txt, QPS file: qps.txt\]', [('deletes', 'd'), ('updates', 'd'), ('inserts', 'd'), ('reads', 'd'), ('duration', 'd')])
            m = until(client_line, data) 
            assert m != False
            return "D/U/I/R = %d/%d/%d/%d Duration = %d" % (m['deletes'], m['updates'], m['inserts'], m['reads'], m['duration'])

    class oprofile_stats():
        oprofile_path   = 'oprofile/oprof.out.rethinkdb'

        def __init__(self, dir):
            self.oprofile  = parser().parse_file(dir + self.oprofile_path)

    def push_html_to_host(self):
        res = open(self.out_dir + '/index.html', 'w')

        print >>res, self.html
        res.close()

        #send stuff to host
        os.system('scp -r "%s" "%s:%s"' % (self.out_dir + '/' + self.dir_str, self.hostname, self.www_dir + self.prof_dir))
        os.system('scp "%s" "%s:%s"' % (self.out_dir + '/' + 'index.html', self.hostname, self.www_dir + self.prof_dir))

    def report_as_html(self):
        def image(source):
            return "<a href=\"%s\"> <img src=\"%s\" height=\"900\" width=\"1200\" /> </a>" % (source, source)

        def flot(source, text):
            return "<a href=\"%s\"> %s </a>" % (self.hostname + self.flot_script_location + '#' + source, text)

        res = StringIO.StringIO()
        print >>res, """<html>"""

        print >>res, "<p> RethinkDB profiling report, autogenerated on %s </p>" % self.dir_str

        flot_data = 'data'
#report stats per run
        for run_name in self.bench_stats.bench_runs.keys():
            run = self.bench_stats.bench_runs[run_name]
            server_meta = self.bench_stats.server_meta[run_name]
            client_meta = self.bench_stats.server_meta[run_name]

            print >>res, '<pre id="#server_meta" style="font-size:large">', server_meta, '</pre>'
            print >>res, '<pre id="#server_meta" style="font-size:large">', client_meta, '</pre>'
            data = reduce(lambda x, y: x + y, run)
#qps plot
            rdb_data = data.select('qps').remap('qps', 'rethinkdb')
            def halve(series):
                return map(lambda x: x/2, series[0])
                
            other_data = rdb_data.copy().derive('Bad Guy', ['rethinkdb'], halve).drop('rethinkdb')
            (rdb_data + other_data).plot(os.path.join(self.out_dir, self.dir_str, 'qps' + run_name))
            print >>res, image(os.path.join(self.hostname, self.prof_dir, self.dir_str, 'qps' + run_name + '.png'))
            print >>res, '<div>', 'Mean qps: %f - stdev: %f' % (data.select('qps').stats()['qps']['mean'], data.select('qps').stats()['qps']['stdev']), '</div>'

#latency histogram
            rdb_data = data.select('latency').remap('latency', 'rethinkdb')
            def double(series):
                return map(lambda x: 2 * x, series[0])

            other_data = rdb_data.copy().derive('Bad Guy', ['rethinkdb'], double).drop('rethinkdb')
            (rdb_data + other_data).histogram(os.path.join(self.out_dir, self.dir_str, 'latency' + run_name))
            print >>res, image(os.path.join(self.hostname, self.prof_dir, self.dir_str, 'latency' + run_name + '.png'))
            print >>res, '<div>', 'Mean latency: %f - stdev: %f' % (data.select('latency').stats()['latency']['mean'], data.select('latency').stats()['latency']['stdev']), '</div>'
#flot link
            data.json(self.out_dir + '/' + self.dir_str + '/' + flot_data + run_name,'Server:' + server_meta + 'Client:' + client_meta)
            print >>res, '<p>'
            print >>res, '<pre>', flot('/' + self.prof_dir + '/' + self.dir_str + '/' + flot_data + run_name + '.js', 'View data for run: %s' % run_name), '</pre>'
            print >>res, '</p>'

#derive cross run data
        try:
            core_runs_names=['c1','c2', 'c4', 'c8', 'c16', 'c32']
            core_runs = {}
            for name in core_runs_names:
                core_runs[name]  = self.bench_stats.bench_runs[name].select('qps').remap('qps', core_run[0] + 'qps')

            means = reduce(lambda x,y: x[1] + y[1], core_runs.iteritems()).derive('qps', map(lambda x : x + 'qps', core_runs_names), means)

#do some plotting
            print >>res, '<p id="#server_meta" style="font-size:large">', 'Core scalability', '</p>'
            means.plot(os.path.join(self.out_dir, self.dir_str, 'core_scalability' + run_name))
            print >>res, image(os.path.join(self.hostname, self.prof_dir, self.dir_str, 'core_scalability' + run_name + '.png'))
            
        except KeyError:
            print 'Not enough core runs to report data, that or a fuck up. Let\'s face it if we were betting men, we\'d bet on it being a fuck up' 
          
        if self.prof_stats:
            prog_report = reduce(lambda x,y: x + y, (map(lambda x: x.oprofile, self.prof_stats)))
            ratios = reduce(lambda x,y: x + y, map(lambda x: x.ratios, small_packet_profiles))
            print >>res, prog_report.report_as_html(ratios, CPU_CLK_UNHALTED, 15)
        else:
            print >>res, "No oprofile data reported"

        print >>res, """</html>"""
        return res.getvalue()

    def send_email(self, recipient):
        print "Sending email to %r..." % recipient
        
        num_tries = 10
        try_interval = 10   # Seconds
        smtp_server, smtp_port = os.environ.get("RETESTER_SMTP", "smtp.gmail.com:587").split(":")
        
        import smtplib

        for tries in range(num_tries):
            try:
                s = smtplib.SMTP(smtp_server, smtp_port)
            except socket.gaierror:
                # Network is being funny. Try again.
                time.sleep(try_interval)
            else:
                break
        else:
            raise Exception("Cannot connect to SMTP server '%s'" % smtp_server)
        
        sender, sender_pw = 'buildbot@rethinkdb.com', 'allspark'
        
        s.starttls()
        s.login(sender, sender_pw)
        header = 'Subject: Profiling results %s \nContent-Type: text/html\n\n' % time.asctime()
        s.sendmail(sender, [recipient], header + self.html)
        s.quit()
        
        print "Email message sent."
