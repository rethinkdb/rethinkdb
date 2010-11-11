import sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir + '/oprofile')))
from plot import *
from oprofile import *
from profiles import *
import time
import locale
import StringIO
from line import *

locale.setlocale(locale.LC_ALL, '')

class dbench():
    log_file = 'bench_log.txt'
    hostname = 'newton'
    www_dir = '/var/www/code.rethinkdb.com/htdocs/'
    prof_dir = 'prof_data' #directory on host where prof data goes
    out_dir = 'bench_html' #local directory to use for data
    bench_dir = 'bench_output'
    oprofile_dir = 'prof_output'
    flot_script_location = '/graph_viewer/index.html'
    competitor_dir = '/home/teapot/competitor_bench'

    def __init__(self, dir, email):
        self.email = email
        self.dir_str = time.asctime().replace(' ', '_').replace(':', '_')
        os.makedirs(self.out_dir + '/' + self.dir_str)
        self.rdb_stats = self.bench_stats(dir + self.bench_dir)
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
#get competitor info
        self.competitors = {}
        competitor_dirs = [dir for dir in os.listdir(self.competitor_dir) if os.path.isdir(os.path.join(self.competitor_dir, dir))]
        for dir in competitor_dirs:
            self.competitors[dir] = self.bench_stats(os.path.join(self.competitor_dir, dir, self.bench_dir))

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
                print 'No bench runs found in: %s' % dir

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
            return "<a href=\"%s\"> <img src=\"%s\" width=\"450\" /> </a>" % (source, source)

        def flot(source, text):
            return "<a href=\"%s\">%s</a>" % ('http://' + self.hostname + self.flot_script_location + '#' + source, text)

        def format_metadata(f):
            return locale.format("%.2f", f, grouping=True)

        res = StringIO.StringIO()


        # Set up basic html, and body tags. Note that the style tag must be under the body tag for email clients to parse it (head gets stripped by most clients).

        print >>res, '<table style="width: 910px; margin-top: 20px; margin-bottom: 20px;"><tr><td style="vertical-align: top;"><h1 style="margin: 0px">RethinkDB performance report</h1></td>'
        print >>res, '<td style="vertical-align: top;"><p style="text-align: right; font-style:italic; margin: 0px;">Report generated on %s</p></td>' % self.dir_str
        print >>res, '</td></tr></table>'

        flot_data = 'data'

        # Report stats for each run
        for run_name in self.rdb_stats.bench_runs.keys():
            run = self.rdb_stats.bench_runs[run_name]
            server_meta = self.rdb_stats.server_meta[run_name]
            client_meta = self.rdb_stats.client_meta[run_name]

            if run_name != self.rdb_stats.bench_runs.keys()[0]:
                print >>res, '<hr style="height: 1px; width: 910px; border-top: 1px solid #999; margin: 30px 0px; padding: 0px 30px;" />'
            print >>res, '<div class="run">'
            print >>res, '<h2 style="font-size: xx-large; display: inline;">', run_name.replace('_',' ') ,'</h2>'

            # Accumulating data for the run
            data = {}
            data['RethinkDB'] = reduce(lambda x, y: x + y, run)
            for competitor in self.competitors.iteritems():
                try:
                    data[competitor[0]] = reduce(lambda x, y: x + y, competitor[1].bench_runs[run_name])
                except KeyError:
                    print 'Bad guy: %s did not report data for run %s' % (competitor[0], run_name)

            # Add a link to the graph-viewer (flot)
            data['RethinkDB'].json(self.out_dir + '/' + self.dir_str + '/' + flot_data + run_name,'Server:' + server_meta + 'Client:' + client_meta)
            print >>res, '<span style="display: inline;">', flot('/' + self.prof_dir + '/' + self.dir_str + '/' + flot_data + run_name + '.js', '(explore data)</span>')
            
            qps_data = TimeSeriesCollection()

            for database in data.iteritems():
                qps_data += database[1].select('qps').remap('qps', database[0])

            # Plot the qps data
            qps_data.plot(os.path.join(self.out_dir, self.dir_str, 'qps' + run_name))
            qps_data.plot(os.path.join(self.out_dir, self.dir_str, 'qps' + run_name + '_large'), True)

            # Add the qps plot image and metadata
            print >>res, '<table style="width: 910px;" class="runPlots">'
            print >>res, '<tr><td><h3 style="text-align: center">Queries per second</h3>'
            print >>res, image('http://' + os.path.join(self.hostname, self.prof_dir, self.dir_str, 'qps' + run_name + '.png'))

            print >>res, """<table style="border-spacing: 0px; border-collapse: collapse; margin-left: auto; margin-right: auto; margin-top: 20px;">
                                <tr style="font-weight: bold; text-align: left; border-bottom: 2px solid #FFFFFF; color: #FFFFFF; background: #556270;">
                                    <th style="padding: 0.5em 0.8em; font-size: small;"></th>
                                    <th style="padding: 0.5em 0.8em; font-size: small;">Mean qps</th>
                                    <th style="padding: 0.5em 0.8em; font-size: small;">Standard deviation</th>
                                </tr>"""
            for competitor in data.iteritems():
                print >>res,     """<tr style="text-align: left; border-bottom: 2px solid #FFFFFF;">
                                        <td style="background: #DBE2F1; padding: 0.5em 0.8em; font-weight: bold;">%s</td>
                                        <td style="background: #DBE2F1; padding: 0.5em 0.8em;">%s</td>
                                        <td style="background: #DBE2F1; padding: 0.5em 0.8em;">%s</td>
                                    </tr>""" % (competitor[0], format_metadata(competitor[1].select('qps').stats()['qps']['mean']), format_metadata(competitor[1].select('qps').stats()['qps']['stdev']))
            print >>res, """</table>
                        </td>"""

            # Build data for the latency histogram
            lat_data = TimeSeriesCollection()

            for competitor in data.iteritems():
                lat_data += competitor[1].select('latency').remap('latency', competitor[0])
            
            # Plot the latency histogram
            lat_data.histogram(os.path.join(self.out_dir, self.dir_str, 'latency' + run_name))

            # Add the latency histogram image and metadata
            print >>res, '<td><h3 style="text-align: center">Latency in microseconds</h3>'
            print >>res, image('http://' + os.path.join(self.hostname, self.prof_dir, self.dir_str, 'latency' + run_name + '.png'))

            print >>res, """<table style="border-spacing: 0px; border-collapse: collapse; margin-left: auto; margin-right: auto; margin-top: 20px;">
                                <tr style="font-weight: bold; text-align: left; border-bottom: 2px solid #FFFFFF; color: #FFFFFF; background: #556270;">
                                    <th style="padding: 0.5em 0.8em; font-size: small;"></th>
                                    <th style="padding: 0.5em 0.8em; font-size: small;">Mean latency</th>
                                    <th style="padding: 0.5em 0.8em; font-size: small;">Standard deviation</th>
                                </tr>"""
            for competitor in data.iteritems():
                print >>res,     """<tr style="text-align: left; border-bottom: 2px solid #FFFFFF;">
                                        <td style="background: #DBE2F1; padding: 0.5em 0.8em; font-weight: bold;">%s</td>
                                        <td style="background: #DBE2F1; padding: 0.5em 0.8em;">%s</td>
                                        <td style="background: #DBE2F1; padding: 0.5em 0.8em;">%s</td>
                                    </tr>""" % (competitor[0], format_metadata(competitor[1].select('latency').stats()['latency']['mean']), format_metadata(competitor[1].select('latency').stats()['latency']['stdev']))
            print >>res, """</table>
                        </td>"""


            # Metadata about the server and client
#            print >>res, '<table style="table-layout: fixed; width: 910px;" class="meta">'
#            print >>res, '<tr><td style="vertical-align: top; width: 50%; padding-right: 40px;"><pre style="font-size: x-small; color: #888;">', server_meta, '</pre></td>'
#            print >>res, '<td style="vertical-align: top; width: 50%; padding-right: 40px;"><pre style="font-size: x-small; color: #888;">', client_meta, '</pre></td></tr>'
#            print >>res, '</table>'

            print >>res, '</div>'

        # Accumulate data across multiple runs, and plot an average for each one (this is for plots where we compare run, e.g. comparing the same workload against different numbers of cores, drives, etc.)
        try:
            core_runs_names=['c1','c2', 'c4', 'c8', 'c16', 'c32']
            core_runs = {}
            for name in core_runs_names:
                core_runs[name]  = reduce(lambda x,y: x+y, self.rdb_stats.bench_runs[name]).select('qps').remap('qps', name + 'qps')

            core_means = reduce(lambda x,y: x + y, map(lambda x: x[1], core_runs.iteritems())).derive('qps', map(lambda x : x + 'qps', core_runs_names), means)

            # Plot the cross-run averages
            print >>res, '<p id="#server_meta" style="font-size:large">', 'Core scalability', '</p>'
            core_means.select('qps').plot(os.path.join(self.out_dir, self.dir_str, 'core_scalability'))
            print >>res, image('http://' + os.path.join(self.hostname, self.prof_dir, self.dir_str, 'core_scalability' + '.png'))
            
        except KeyError:
            print 'Not enough core runs to report data, that or a fuck up. Let\'s face it if we were betting men, we\'d bet on it being a fuck up' 

        
        # Add oprofile data
#        print >> res, '<div class="oprofile">' 
#        if self.prof_stats:
#            prog_report = reduce(lambda x,y: x + y, (map(lambda x: x.oprofile, self.prof_stats)))
#            ratios = reduce(lambda x,y: x + y, map(lambda x: x.ratios, small_packet_profiles))
#            print >>res, prog_report.report_as_html(ratios, CPU_CLK_UNHALTED, 15)
#        else:
#            print >>res, "<p>No oprofile data reported</p>"
#        print >> res, '</div>' 
          

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
