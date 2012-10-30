# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/python
import sys, os, signal, math
import _mysql

from statsPlotter import *
import tornado.httpserver
import tornado.ioloop
import tornado.web
import urllib
import time
import datetime


class PlotTemplate(tornado.web.RequestHandler):
    # TODO: Use tornado templates for this stuff...
    def head(self, title = None):
        self.write('<html><head>')
        if title:
            self.write('<title>%s</title>' % title)
        self.write('</head><body bgcolor="#404040" text="#ffffff" link="#eeeeee" alink="#ffffff" vlink="#eeeeee">')
        if title:
            self.write('<h1>%s</h1>' % title)

    def tail(self):
        self.write('</body></html>')

class PlotSelectHandler(PlotTemplate):
    def get(self):
        self.head("Build your plot!")

        self.write('<form method="GET" action="configurePlot.html">')
        self.write('<table border="0">')

        self.write('<tr><td>Name of plot:</td><td><input name="name" value="plot"></td></tr>')
        self.write('<tr><td>Test run:</td><td><select name="run">')
        db_conn = _mysql.connect("newton", "longtest", "rethinkdb2010", "longtest") # TODO
        db_conn.query("SELECT `run` FROM `stats` GROUP BY `run` ORDER BY `run` DESC")
        result = db_conn.use_result()
        rows = result.fetch_row(maxrows=0) # Fetch all rows
        for row in rows:
            run_timestamp = int(row[0])
            self.write('<option value="%d">%s</option>' % (run_timestamp, str(datetime.datetime.fromtimestamp(run_timestamp))))
        db_conn.close()
        self.write('</select></td></tr>')
        self.write('<tr><td>End plot</td><td><input name="end_hours" value="0" size="3"> hours before the latest available data of the run</td></tr>')
        self.write('<tr><td>...and start</td><td><input name="duration" value="6" size="3"> hours before that point.</td></tr>')

        #self.write('<tr><td>End timestamp:</td><td><input name="end_timestamp" value="%d"></td></tr>' % time.time())
        #self.write('<tr><td>Plot duration:</td><td><input name="duration" value="6" size="3"> hours</td></tr>')
        self.write('<tr><td>Plotter type:</td><td><select name="plotter_type"> \
            <option value="simple_plotter">simple plotter</option> \
            <option value="differential_plotter">differential plotter</option> \
            <option value="two_stats_diff_plotter">two stats differential plotter</option> \
            <option value="two_stats_ratio_plotter">two stats ratio plotter</option> \
            </select></td></tr>')
        self.write('<tr><td>Plot style:</td><td><select name="plot_style"> \
            <option value="lines">lines</option> \
            <option value="quantile">quantiles (linear)</option> \
            <option value="quantilel">quantiles (logarithmic)</option> \
            </select></td></tr>')

        self.write('</table>')
        self.write('<br><input type="submit" value="Continue">')
        self.write('</form>')

        self.tail()
        
class PlotConfigureHandler(PlotTemplate):
    stats_for_display = [
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
    ]

    def get(self):
        name = self.get_argument("name")
        run = self.get_argument("run")
        db_conn = _mysql.connect("newton", "longtest", "rethinkdb2010", "longtest") # TODO
        db_conn.query("SELECT `timestamp` FROM `stats` WHERE `run` = '%s' ORDER BY `timestamp` DESC LIMIT 1" % db_conn.escape_string(run))
        result = db_conn.use_result()
        rows = result.fetch_row(maxrows=0) # Fetch all rows
        latest_timestamp = int(rows[0][0])
        db_conn.close()
        end_timestamp = str(int(latest_timestamp) - int(float(self.get_argument("end_hours")) * 3600.0))
        start_timestamp = str(int(end_timestamp) - int(float(self.get_argument("duration")) * 3600.0))
        plotter_type = self.get_argument("plotter_type")
        plot_style = self.get_argument("plot_style")

        self.head()

        self.write('<form method="GET" action="generatePlot.html">')
        self.write('<table border="0">')

        if plot_style == "quantile" or plot_style == "quantilel":
            self.write('<tr><td>Bucket size:</td><td><input name="quantile_bucket_size" value="20" size="5"></td></tr>')
            self.write('<tr><td>Ranges:</td><td><input name="quantile_ranges" value="0.05,0.5,0.95"></td></tr>')

        if plotter_type == "simple_plotter":
            self.write('<tr><td>Stat:</td><td><select name="simple_plotter_stat">')
            for stat in self.stats_for_display:
                self.write('<option value="%s">%s</option>' % (stat, stat) )
            self.write('</select></td></tr>')
            self.write('<tr><td>Multiplier:</td><td><input name="simple_plotter_multiplier" value="1" size="5"></td></tr>')
        elif plotter_type == "differential_plotter":
            self.write('<tr><td>Stat:</td><td><select name="differential_plotter_stat">')
            for stat in self.stats_for_display:
                self.write('<option value="%s">%s</option>' % (stat, stat) )
            self.write('</select></td></tr>')
        elif plotter_type == "two_stats_diff_plotter":
            self.write('<tr><td>Stat 1:</td><td><select name="two_stats_diff_plotter_stat1">')
            for stat in self.stats_for_display:
                self.write('<option value="%s">%s</option>' % (stat, stat) )
            self.write('</select></td></tr>')
            self.write('<tr><td>Stat 2:</td><td><select name="two_stats_diff_plotter_stat2">')
            for stat in self.stats_for_display:
                self.write('<option value="%s">%s</option>' % (stat, stat) )
            self.write('</select></td></tr>')
        elif plotter_type == "two_stats_ratio_plotter":
            self.write('<tr><td>Dividend:</td><td><select name="two_stats_ratio_plotter_dividend">')
            for stat in self.stats_for_display:
                self.write('<option value="%s">%s</option>' % (stat, stat) )
            self.write('</select></td></tr>')
            self.write('<tr><td>Divisor:</td><td><select name="two_stats_ratio_plotter_divisor">')
            for stat in self.stats_for_display:
                self.write('<option value="%s">%s</option>' % (stat, stat) )
            self.write('</select></td></tr>')

        self.write('</table>')
        for (hidden_name, hidden_value) in [
                ("name", name),
                ("plotter_type", plotter_type),
                ("plot_style", plot_style),
                ("start_timestamp", start_timestamp),
                ("run", run),
                ("end_timestamp", end_timestamp)]:
            self.write('<input type="hidden" name="%s" value="%s">' % (hidden_name, hidden_value) ); # Cross site scripting vulnerability! ;-)
        self.write('<br><input type="submit" value="Generate">')
        self.write('</form>')

        self.tail()

class PlotGeneratorHandler(PlotTemplate):
    def get(self):
        name = self.get_argument("name")
        pass_values = [("name", name),
            ("start_timestamp", self.get_argument("start_timestamp")),
            ("end_timestamp", self.get_argument("end_timestamp")),
            ("run", self.get_argument("run")),
            ("plotter_type", self.get_argument("plotter_type"))]

        plot_style = self.get_argument("plot_style")
        if plot_style == "quantile" or plot_style == "quantilel":
            quantile_bucket_size = self.get_argument("quantile_bucket_size")
            quantile_ranges = self.get_argument("quantile_ranges")
            plot_style = "%s %d %s" % (plot_style, int(quantile_bucket_size), quantile_ranges)
        pass_values.append(("plot_style", plot_style))

        plotter_type = self.get_argument("plotter_type")
        pass_values.append(("plotter_type", plotter_type))
        if plotter_type == "simple_plotter":
            pass_values.append(("simple_plotter_stat", self.get_argument("simple_plotter_stat")))
            pass_values.append(("simple_plotter_multiplier", self.get_argument("simple_plotter_multiplier")))
        elif plotter_type == "differential_plotter":
            pass_values.append(("differential_plotter_stat", self.get_argument("differential_plotter_stat")))
        elif plotter_type == "two_stats_diff_plotter":
            pass_values.append(("two_stats_diff_plotter_stat1", self.get_argument("two_stats_diff_plotter_stat1")))
            pass_values.append(("two_stats_diff_plotter_stat2", self.get_argument("two_stats_diff_plotter_stat2")))
        elif plotter_type == "two_stats_ratio_plotter":
            pass_values.append(("two_stats_ratio_plotter_dividend", self.get_argument("two_stats_ratio_plotter_dividend")))
            pass_values.append(("two_stats_ratio_plotter_divisor", self.get_argument("two_stats_ratio_plotter_divisor")))

        self.head()

        plot_url = "plot.png?" + urllib.urlencode(pass_values)
        window_open_js = "window.open('%s','Plot - %s (generated at %d)', 'menubar=no,width=1054,height=158,toolbar=no');" % (plot_url, name, time.time())

        self.write('Your plot is being generated...<br>')
        self.write('If it does not open automatically, <a target="_blank" onClick="%s return false" href="%s">click here</a> (please enable popup windows for this site).<br><br>Set up <a href="selectPlot.html">another plot</a>.' % (window_open_js, plot_url))

        self.write('<script language="JavaScript">')
        self.write(window_open_js)
        self.write('</script>')

        self.tail()

class PlotHandler(tornado.web.RequestHandler):
    def get(self):
        # Get arguments
        start_timestamp = self.get_argument("start_timestamp")
        end_timestamp = self.get_argument("end_timestamp")
        run = self.get_argument("run")
        name = self.get_argument("name")
        plot_style = self.get_argument("plot_style", "quantile 20 0.05,0.5,0.95")
        plotter_type = self.get_argument("plotter_type", "simple_plotter")
        if plotter_type == "simple_plotter":
            stat = self.get_argument("simple_plotter_stat", name)
            multiplier = float(self.get_argument("simple_plotter_multiplier", "1"))
            plotter = simple_plotter(stat, multiplier)
        elif plotter_type == "differential_plotter":
            stat = self.get_argument("differential_plotter_stat", name)
            plotter = differential_plotter(stat)
        elif plotter_type == "two_stats_diff_plotter":
            stat1 = self.get_argument("two_stats_diff_plotter_stat1")
            stat2 = self.get_argument("two_stats_diff_plotter_stat2")
            plotter = two_stats_diff_plotter(stat1, stat2)
        elif plotter_type == "two_stats_ratio_plotter":
            dividend = self.get_argument("two_stats_ratio_plotter_dividend")
            divisor = self.get_argument("two_stats_ratio_plotter_divisor")
            plotter = two_stats_ratio_plotter(dividend, divisor)
        else:
            # TODO: Handle error condition
            raise Exception("Unsupported plotter_type: %s" % plotter_type)

        # Generate plot
        plot = DBPlot(run, start_timestamp, end_timestamp, name, plot_style, plotter)

        # Send the response...
        self.set_header("Content-Type", "image/png")
        self.write(plot.get_plot())

application = tornado.web.Application([
    (r"/", PlotSelectHandler),
    (r"/plot.png", PlotHandler),
    (r"/selectPlot.html", PlotSelectHandler),
    (r"/configurePlot.html", PlotConfigureHandler),
    (r"/generatePlot.html", PlotGeneratorHandler),
])

if __name__ == "__main__":
    port = 8899
    http_server = tornado.httpserver.HTTPServer(application)
    http_server.listen(port)
    print "Listening on port %d" % port
    tornado.ioloop.IOLoop.instance().start()


