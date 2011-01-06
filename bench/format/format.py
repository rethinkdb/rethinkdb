import sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir + '/oprofile')))
from plot import *
from oprofile import *
from profiles import *
import time
import locale
import StringIO
from line import *
from email.MIMEMultipart import MIMEMultipart
from email.MIMEText import MIMEText
from email.MIMEImage import MIMEImage  
import pdb

locale.setlocale(locale.LC_ALL, '')

class Run(object):
    def __init__(self, name = '', data = None, server_meta = None, client_meta = None):
        self.name = name.replace('_', ' ')
        self.data = data
        self.server_meta = server_meta
        self.client_meta = client_meta

class Multirun(object):
    def __init__(self, name = '', runs = {}, unit = None):
        self.name = name.replace('_', ' ')
        self.runs = runs
        self.unit = unit

#        print means([runs[run].data.select('iostat') for run in runs]) 

  #      self.data = multirun_data

class dbench():
    log_file = 'bench_log.txt'
    hostname = 'newton'
    www_dir = '/var/www/code.rethinkdb.com/htdocs/'
    prof_dir = 'prof_data' #directory on host where prof data goes
    out_dir = 'bench_html' #local directory to use for data
    bench_dir = 'bench_output'
    oprofile_dir = 'prof_output'
    flot_script_location = '/graph_viewer/index.html'
    competitor_dir = os.getenv("HOME", "/home/teapot") + '/competitor_bench'

    def __init__(self, dir, email_addr):
        self.email_addr = email_addr
        self.dir_str = time.asctime().replace(' ', '_').replace(':', '_')
        os.makedirs(self.out_dir + '/' + self.dir_str)
        self.rdb_stats = self.bench_stats(dir + self.bench_dir)
        self.images_used = []

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

        # Build competitor stats
        self.competitors = {}

        # Collect just the directories containing competitor data
        competitor_dirs = []
        try:
            competitor_dirs = [name for name in os.listdir(self.competitor_dir) if os.path.isdir(os.path.join(self.competitor_dir, name))]
        except:
            print 'No competitors found in: '+self.competitor_dir

        for dir in competitor_dirs:
            self.competitors[dir] = self.bench_stats(os.path.join(self.competitor_dir, dir, self.bench_dir))

    def report(self):
        self.html = self.report_as_html()
        self.push_html_to_host()
        self.send_email(self.email_addr)
        os.system('rm -rf %s' % self.out_dir)

    class bench_stats():
        iostat_path     = 'iostat/output.txt'
        vmstat_path     = 'vmstat/output.txt'
        latency_path    = 'client/latency.txt'
        qps_path        = 'client/qps.txt'
        rdbstat_path    = 'rdbstat/output.txt'
        server_meta_path= 'server/output.txt'
        client_meta_path= 'client/output.txt'
        multirun_flag   = 'multirun'        

        def __init__(self, dir):
            dir = os.path.normpath(dir)
            # Get the list of all runs and multiruns
            singlerun_dirs = []
            multirun_dirs = []
            dir_listing = []

            # Get the current list of just subdirectories (each representing a run / multirun)
            try:
                dir_listing = [name for name in os.listdir(dir) if os.path.isdir(os.path.join(dir,name))]
            except:
                print 'No bench runs found in: %s' % dir

            # Sort each run as either a single run or multirun
            for curr_dir in dir_listing:

                # Check if the current directory has run or multirun data
                if os.path.isfile(os.path.join(dir,curr_dir,self.multirun_flag)):
                    multirun_dirs.append(curr_dir)
                else:
                    singlerun_dirs.append(curr_dir)

            self.single_runs = {}
            self.multi_runs = {}
            
            # Defined function: Collect data from the directory provided, store the data in the given parent (either generic runs or multiruns).
            def collect_run_data(run, run_dir, parent):
                run_data  = [IOStat().read(os.path.join(run_dir, self.iostat_path)),
                            VMStat().read(os.path.join(run_dir, self.vmstat_path)),
                            Latency().read(os.path.join(run_dir, self.latency_path)),
                            QPS().read(os.path.join(run_dir, self.qps_path)),
                            RDBStats().read(os.path.join(run_dir, self.rdbstat_path))]

                try:
                    server_meta = (open(os.path.join(run_dir, self.server_meta_path)).read())
                except IOError:
                    server_meta = ''
                    print "No server metadata found for run: "+run.replace('_',' ')

                try:
                    client_meta = (open(os.path.join(run_dir, self.client_meta_path)).read())
                except IOError:
                    client_meta = ''
                    print "No client metadata found for run: "+run.replace('_',' ')

                parent[run] = Run(run, run_data, server_meta, client_meta)

            # Collect and built single data and metadata
            for singlerun in singlerun_dirs:
                collect_run_data(singlerun, os.path.join(dir,singlerun,'1'), self.single_runs)

            # Collect and built multirun data and metadata
            for multirun in multirun_dirs:
                multirun_dir = os.path.join(dir, multirun)

                # Read in the units from the multirun file (should be the first line)
                try:
                    unit = open(os.path.join(multirun_dir, self.multirun_flag)).readline()
                except IOError:
                    unit = ''
                    print "Could not read units for multirun: "+multirun

                # Each multirun consists of several runs. Read in each run's data, and add it to the given multirun.
                run_dirs = []

                try:
                    # Make sure we only include directories
                    run_dirs = [name for name in os.listdir(multirun_dir) if os.path.isdir(os.path.join(multirun_dir,name))]
                except:
                    print 'Multirun has no valid runs: '+multirun_dir

                runs = {}
                
                # Collect the run data across all multirun runs
                for run in run_dirs:
                    collect_run_data(run,os.path.join(multirun_dir,run,'1'), runs)

                # Create a new Multirun with the collected data
                self.multi_runs[multirun] = Multirun(multirun, runs, unit)
                
                # Determine the mean of each run, create a new TimeSeriesMeans object so we can plot the means later
                multirun_data = []
                for run_name in self.multi_runs[multirun].runs:
                    run = self.multi_runs[multirun].runs[run_name]
                    multirun_data.append(reduce(lambda x,y: x+y, run.data).select('qps').remap('qps',run.name))

                if multirun_data == []:
                    print "Did not get multirun_data for %s" % multirun
                else:
                    self.multi_runs[multirun].data = TimeSeriesMeans(multirun_data)

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
        def image(name):
            # Construct the path to the high-resolution version of the plot, append the plot image to the list of images to be attached.
            large_img_path = 'http://'+os.path.join(self.hostname, self.prof_dir, self.dir_str, name+'_large.png')
            self.images_used.append(name)
            return "<a href=\"%s\"> <img src=\"%s\" width=\"450\" /> </a>" % (large_img_path, 'cid:'+name)

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
        for run_name in self.rdb_stats.single_runs.keys():
            run = self.rdb_stats.single_runs[run_name]
            server_meta = run.server_meta
            client_meta = run.client_meta

            if run_name != self.rdb_stats.single_runs.keys()[0]:
                print >>res, '<hr style="height: 1px; width: 910px; border-top: 1px solid #999; margin: 30px 0px; padding: 0px 30px;" />'
            print >>res, '<div class="run">'
            print >>res, '<h2 style="font-size: xx-large; display: inline;">', run.name,'</h2>'

            # Accumulating data for the run
            data = {}
            data['RethinkDB'] = reduce(lambda x, y: x + y, run.data)

            # Accumulating data for competitors' run
            for competitor in self.competitors.iteritems():
                try:
                    data[competitor[0]] = reduce(lambda x, y: x + y, competitor[1].single_runs[run_name].data)
                except KeyError:
                    print 'Competitor: %s did not report data for run %s' % (competitor[0], run.name)

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
            print >>res, image('qps' + run_name)

            print >>res, """<table style="border-spacing: 0px; border-collapse: collapse; margin-left: auto; margin-right: auto; margin-top: 20px;">
                                <tr style="font-weight: bold; text-align: left; border-bottom: 2px solid #FFFFFF; color: #FFFFFF; background: #556270;">
                                    <th style="padding: 0.5em 0.8em; font-size: small;"></th>
                                    <th style="padding: 0.5em 0.8em; font-size: small;">Mean qps</th>
                                    <th style="padding: 0.5em 0.8em; font-size: small;">Standard deviation</th>
                                    <th style="padding: 0.5em 0.8em; font-size: small;">Upper / Lower 5-percentile</th>
                                </tr>"""
            for competitor in data.iteritems():
                try:
                    mean_qps = format_metadata(competitor[1].select('qps').stats()['qps']['mean'])
                except KeyError:
                    mean_qps = "N/A"
                try:
                    standard_dev = format_metadata(competitor[1].select('qps').stats()['qps']['stdev'])
                except KeyError:
                    standard_dev = "N/A"
                try:
                    upper_percentile = format_metadata(competitor[1].select('qps').stats()['qps']['upper_5_percentile'])
                    lower_percentile = format_metadata(competitor[1].select('qps').stats()['qps']['lower_5_percentile'])
                except KeyError:
                    upper_percentile = "N/A"
                    lower_percentile = "N/A"
                print >>res,     """<tr style="text-align: left; border-bottom: 2px solid #FFFFFF;">
                                        <td style="background: #DBE2F1; padding: 0.5em 0.8em; font-weight: bold;">%s</td>
                                        <td style="background: #DBE2F1; padding: 0.5em 0.8em;">%s</td>
                                        <td style="background: #DBE2F1; padding: 0.5em 0.8em;">%s</td>
                                        <td style="background: #DBE2F1; padding: 0.5em 0.8em;">%s / %s</td>
                                    </tr>""" % (competitor[0], mean_qps, standard_dev, upper_percentile, lower_percentile)
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
            print >>res, image('latency' + run_name)

            print >>res, """<table style="border-spacing: 0px; border-collapse: collapse; margin-left: auto; margin-right: auto; margin-top: 20px;">
                                <tr style="font-weight: bold; text-align: left; border-bottom: 2px solid #FFFFFF; color: #FFFFFF; background: #556270;">
                                    <th style="padding: 0.5em 0.8em; font-size: small;"></th>
                                    <th style="padding: 0.5em 0.8em; font-size: small;">Mean latency</th>
                                    <th style="padding: 0.5em 0.8em; font-size: small;">Standard deviation</th>
                                    <th style="padding: 0.5em 0.8em; font-size: small;">Upper / Lower 5-percentile</th>
                                </tr>"""
            for competitor in data.iteritems():
                try:
                    mean_latency = format_metadata(competitor[1].select('latency').stats()['latency']['mean'])
                except KeyError:
                    mean_latency = "N/A"
                try:
                    standard_dev = format_metadata(competitor[1].select('latency').stats()['latency']['stdev'])
                except KeyError:
                    standard_dev = "N/A"
                try:
                    upper_percentile = format_metadata(competitor[1].select('latency').stats()['latency']['upper_5_percentile'])
                    lower_percentile = format_metadata(competitor[1].select('latency').stats()['latency']['lower_5_percentile'])
                except KeyError:
                    upper_percentile = "N/A"
                    lower_percentile = "N/A"
                print >>res,     """<tr style="text-align: left; border-bottom: 2px solid #FFFFFF;">
                                        <td style="background: #DBE2F1; padding: 0.5em 0.8em; font-weight: bold;">%s</td>
                                        <td style="background: #DBE2F1; padding: 0.5em 0.8em;">%s</td>
                                        <td style="background: #DBE2F1; padding: 0.5em 0.8em;">%s</td>
                                        <td style="background: #DBE2F1; padding: 0.5em 0.8em;">%s / %s</td>
                                    </tr>""" % (competitor[0], mean_latency, standard_dev, upper_percentile, lower_percentile)
            print >>res, """</table>
                        </td>"""


            # Metadata about the server and client
#            print >>res, '<table style="table-layout: fixed; width: 910px;" class="meta">'
#            print >>res, '<tr><td style="vertical-align: top; width: 50%; padding-right: 40px;"><pre style="font-size: x-small; color: #888;">', server_meta, '</pre></td>'
#            print >>res, '<td style="vertical-align: top; width: 50%; padding-right: 40px;"><pre style="font-size: x-small; color: #888;">', client_meta, '</pre></td></tr>'
#            print >>res, '</table>'

            print >>res, '</div>'
        
        # Report stats for each multirun
        for multirun_name in self.rdb_stats.multi_runs.keys():
            multirun = self.rdb_stats.multi_runs[multirun_name]

            print >>res, '<hr style="height: 1px; width: 910px; border-top: 1px solid #999; margin: 30px 0px; padding: 0px 30px;" />'
            print >>res, '<div class="multi run">'
            print >>res, '<h2 style="font-size: xx-large; display: inline;">', multirun.name,'</h2>'

            # Get the data for the multirun mean scatter plot
            mean_data = {}
            mean_data['RethinkDB'] = multirun.data

            for competitor in self.competitors.iteritems():
                try:
                    mean_data[competitor[0]] = competitor[1].multi_runs[multirun_name].data
                except KeyError:
                    print 'Competitor: %s did not report mean data for multirun %s' % (competitor[0], multirun.name) 
                except AttributeError:
                    print 'Competitor: %s has no multiruns.' % competitor[0]

            # REMOVED PENDING BETTER IMPLEMENTATION Add a link to the graph-viewer (flot) TODO
#            data.json(self.out_dir + '/' + self.dir_str + '/' + flot_data + run_name,'Server:' + server_meta + 'Client:' + client_meta)
#            print >>res, '<span style="display: inline;">', flot('/' + self.prof_dir + '/' + self.dir_str + '/' + flot_data + run_name + '.js', '(explore data)</span>')
            

            # Check if we can use the labels as x values (i.e. they are all numeric)
            labels_are_x_values = True
            try:
                for db_name in mean_data.keys():
                    current_data = mean_data[db_name].scatter
                    for i, label in current_data.names.iteritems():
                        float(label)
            except ValueError:
                labels_are_x_values = False

            # Plot the mean run data
            scatter_data = {}
            for db_name in mean_data.keys():
                current_data = mean_data[db_name].scatter
                scatter_data[db_name] = []
                if labels_are_x_values:
                    for i, label in current_data.names.iteritems():
                        current_data.data[i] = (float(label), current_data.data[i][1])
                    # Sort data points before plotting
                    current_data.data.sort()
                for i, label in current_data.names.iteritems():
                    scatter_data[db_name].append(current_data.data[i])

            scatter = ScatterCollection(scatter_data, multirun.unit)
            scatter.plot(os.path.join(self.out_dir, self.dir_str, 'mean' + multirun_name))
#            mean_data.plot(os.path.join(self.out_dir, self.dir_str, 'meanrun' + multirun_name + '_large'), True)

            # Add the mean run plot image and metadata
            print >>res, '<table style="width: 910px;" class="runPlots">'
            print >>res, '<tr><td><h3 style="text-align: center">Queries per second</h3>'
            print >>res, image('mean' + multirun_name)

            # REMOVED PENDING REVIEW TODO
            #cum_stats = {}
            #cum_stats['rdb_qps_mean'] = format_metadata(data.select('qps').stats()['qps']['mean'])
            #cum_stats['rdb_qps_stdev'] = format_metadata(data.select('qps').stats()['qps']['stdev'])

            # REMOVED PENDING REIVEW TODO
#            for competitor in competitor_data.iteritems():
#                cum_stats[competitor[0] + '_qps_mean'] = format_metadata(competitor[1].select('qps').stats()['qps']['mean'])
#                cum_stats[competitor[0] + '_qps_stdev']= format_metadata(competitor[1].select('qps').stats()['qps']['stdev'])

#            qps_mean = format_metadata(data.select('qps').stats()['qps']['mean'])
#            qps_stdev = format_metadata(data.select('qps').stats()['qps']['stdev'])
#            print >>res, """<table style="border-spacing: 0px; border-collapse: collapse; margin-left: auto; margin-right: auto; margin-top: 20px;">
#                                <tr style="font-weight: bold; text-align: left; border-bottom: 2px solid #FFFFFF; color: #FFFFFF; background: #556270;">
#                                    <th style="padding: 0.5em 0.8em; font-size: small;"></th>
#                                    <th style="padding: 0.5em 0.8em; font-size: small;">Mean qps</th>
#                                    <th style="padding: 0.5em 0.8em; font-size: small;">Standard deviation</th>
#                                </tr>
#                                <tr style="text-align: left; border-bottom: 2px solid #FFFFFF;">
#                                    <td style="background: #DBE2F1; padding: 0.5em 0.8em; font-weight: bold;">RethinkDB</td>
#                                    <td style="background: #DBE2F1; padding: 0.5em 0.8em;">%s</td>
#                                    <td style="background: #DBE2F1; padding: 0.5em 0.8em;">%s</td>
#                                </tr>
#                                <tr style="text-align: left; border-bottom: 2px solid #FFFFFF;">
#                                    <td style="background: #DBE2F1; padding: 0.5em 0.8em; font-weight: bold;">Membase</td>
#                                    <td style="background: #DBE2F1; padding: 0.5em 0.8em;">%s</td>
#                                    <td style="background: #DBE2F1; padding: 0.5em 0.8em;">%s</td>
#                                </tr>
#                                <tr style="text-align: left; border-bottom: 2px solid #FFFFFF;">
#                                    <td style="background: #DBE2F1; padding: 0.5em 0.8em; font-weight: bold;">MySQL</td>
#                                    <td style="background: #DBE2F1; padding: 0.5em 0.8em;">%s</td>
#                                    <td style="background: #DBE2F1; padding: 0.5em 0.8em;">%s</td>
#                                </tr>
#                            </table>
#                        </td>
#                        """ % (cum_stats.get('rdb_qps_mean', '8===D'), cum_stats.get('rdb_qps_stdev', '8===D'), cum_stats.get('membase_qps_mean', '8===D'), cum_stats.get('membase_qps_stdev', '8===D'), cum_stats.get('mysql_qps_mean', '8===D'), cum_stats.get('mysql_qps.stdev', '8===D'))

            # Build data for the multiplot
            #multiplot_data = data.select('latency').remap('latency', 'rethinkdb')

            # REMOVED PENDING REVIEW TODO
#            for competitor in competitor_data.iteritems():
#                lat_data += competitor[1].select('latency').remap('latency', competitor[0])
            
            # Plot the multiplot 
            #multiplot_data.histogram(os.path.join(self.out_dir, self.dir_str, 'latency' + run_name))

            # REMOVED PENDING REVIEW Add the latency histogram image and metadata TODO
#            print >>res, '<td><h3 style="text-align: center">Latency in microseconds</h3>'
#            print >>res, image('latency' + run_name)
#
#            latency_mean = format_metadata(data.select('latency').stats()['latency']['mean'])
#            latency_stdev = format_metadata(data.select('latency').stats()['latency']['stdev'])
#            print >>res, """<table style="border-spacing: 0px; border-collapse: collapse; margin-left: auto; margin-right: auto; margin-top: 20px;">
#                                <tr style="font-weight: bold; text-align: left; border-bottom: 2px solid #FFFFFF; color: #FFFFFF; background: #556270;">
#                                    <th style="padding: 0.5em 0.8em; font-size: small;"></th>
#                                    <th style="padding: 0.5em 0.8em; font-size: small;">Mean latency</th>
#                                    <th style="padding: 0.5em 0.8em; font-size: small;">Standard deviation</th>
#                                </tr>
#                                <tr style="text-align: left; border-bottom: 2px solid #FFFFFF;">
#                                    <td style="background: #DBE2F1; padding: 0.5em 0.8em; font-weight: bold;">RethinkDB</td>
#                                    <td style="background: #DBE2F1; padding: 0.5em 0.8em;">%s</td>
#                                    <td style="background: #DBE2F1; padding: 0.5em 0.8em;">%s</td>
#                                </tr>
#                                <tr style="text-align: left; border-bottom: 2px solid #FFFFFF;">
#                                    <td style="background: #DBE2F1; padding: 0.5em 0.8em; font-weight: bold;">Membase</td>
#                                    <td style="background: #DBE2F1; padding: 0.5em 0.8em;">%s</td>
#                                    <td style="background: #DBE2F1; padding: 0.5em 0.8em;">%s</td>
#                                </tr>
#                                <tr style="text-align: left; border-bottom: 2px solid #FFFFFF;">
#                                    <td style="background: #DBE2F1; padding: 0.5em 0.8em; font-weight: bold;">MySQL</td>
#                                    <td style="background: #DBE2F1; padding: 0.5em 0.8em;">%s</td>
#                                    <td style="background: #DBE2F1; padding: 0.5em 0.8em;">%s</td>
#                                </tr>
#                            </table>
#                        </td>
#                        """ % (latency_mean, latency_stdev, latency_mean, latency_stdev, latency_mean, latency_stdev)
#            print >>res, '</tr></table>'



            # Metadata about the server and client
#            print >>res, '<table style="table-layout: fixed; width: 910px;" class="meta">'
#            print >>res, '<tr><td style="vertical-align: top; width: 50%; padding-right: 40px;"><pre style="font-size: x-small; color: #888;">', server_meta, '</pre></td>'
#            print >>res, '<td style="vertical-align: top; width: 50%; padding-right: 40px;"><pre style="font-size: x-small; color: #888;">', client_meta, '</pre></td></tr>'
#            print >>res, '</table>'

            print >>res, '</div>'
            
        # Accumulate data across multiple runs, and plot an average for each one (this is for plots where we compare run, e.g. comparing the same workload against different numbers of cores, drives, etc.)
#        try:
#            core_runs_names=['c1','c2', 'c4', 'c8', 'c16', 'c32']
#            core_runs = {}
#            for name in core_runs_names:
#                core_runs[name]  = reduce(lambda x,y: x+y,run.data).select('qps').remap('qps', name + 'qps')
#
#            core_means = reduce(lambda x,y: x + y, map(lambda x: x[1], core_runs.iteritems())).derive('qps', map(lambda x : x + 'qps', core_runs_names), means)
#
#            # Plot the cross-run averages
#            print >>res, '<p id="#server_meta" style="font-size:large">', 'Core scalability', '</p>'
#            core_means.select('qps').plot(os.path.join(self.out_dir, self.dir_str, 'core_scalability'))
#            print >>res, image('core_scalability')
#            
#        except KeyError:
#            print 'Not enough core runs to report data, that or a fuck up. Let\'s face it if we were betting men, we\'d bet on it being a fuck up' 

        
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
        
        # Build a basic MIME multipart message (html / text)
        msg = MIMEMultipart('alternative')
        msg['Subject'] = 'Profiling results %s' % time.asctime()
        msg['From'] = 'buildbot@rethinkdb.com'
        msg['To'] = recipient

        # Attach both a plain-text and html version of the message
        msg.attach(MIMEText('Profiling reports can only be viewed by a client that supports HTML.', 'plain'))
        msg.attach(MIMEText(self.html, 'html'))

        for image in self.images_used:
            fp = open(os.path.join(self.out_dir,self.dir_str,image+'.png'), 'rb')
            msg_img = MIMEImage(fp.read())
            fp.close()
            msg_img.add_header('Content-ID', '<'+image+'>')
            msg.attach(msg_img)

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
        s.sendmail(sender, [recipient], msg.as_string())
        s.quit()
        
        print "Email message sent."
