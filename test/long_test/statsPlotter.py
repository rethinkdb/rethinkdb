#!/usr/bin/python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import sys, os, signal, math
import _mysql
from datetime import datetime
from subprocess import Popen, PIPE

def simple_plotter(name, multiplier = 1):
    return lambda (old, new): new[name]*multiplier if name in new and new[name] else None

def differential_plotter(name):
    return lambda (old, new): (new[name]-old[name])/(new['uptime']-old['uptime']) if name in new and name in old and new[name] and old[name] else None

def two_stats_diff_plotter(stat1, stat2):
    return lambda (old, new): new[stat1]-new[stat2] if stat1 in new and stat2 in new and new[stat1] and new[stat2] else None

def two_stats_ratio_plotter(dividend, divisor):
    return lambda (old, new): new[dividend]/new[divisor] if dividend in new and divisor in new and new[dividend] and new[divisor] and new[divisor] != 0 else None

class Plot(object):
    single_plot_height = 128

    def __init__(self, stats, name, plot_style, plotter):
        self.name = name
        self.plot_style = plot_style
        self.plotter = plotter
        self.plot = None
        self.stats = stats

    def get_plot(self):
        if not self.plot:
            self.plot = self.generate_plot(self.stats)
        return self.plot

    def generate_plot(self, all_stats):
        zipped_stats = zip(all_stats, all_stats[1:])
        plot_instructions = ['-k', self.name, self.plot_style]

        tplot = Popen(['tplot',
            '-if', '-',
            '-o', '/dev/stdout',
            '-of', 'png',
            '-tf', 'num',
            '-or', '1024x%d' % (Plot.single_plot_height),
            ] + plot_instructions, stdin=PIPE, stdout=PIPE)

        for stats_pair in zipped_stats:
            val = self.plotter(stats_pair)
            if val:
                ts = stats_pair[1]['uptime']
                print >>tplot.stdin, "%f =%s %f" % (ts, self.name, val)

        tplot.stdin.close()
        return tplot.stdout.read()

class DBPlot(Plot):
    def __init__(self, run, start_timestamp, end_timestamp, name, plot_style, plotter):
        self.db_conn = _mysql.connect("newton", "longtest", "rethinkdb2010", "longtest") # TODO
        stats = self.load_stats_from_db(run, start_timestamp, end_timestamp)
        self.db_conn.close()

        Plot.__init__(self, stats, name, plot_style, plotter)

    def load_stats_from_db(self, run, start_timestamp, end_timestamp):
        start_timestamp = self.db_conn.escape_string(start_timestamp)
        end_timestamp = self.db_conn.escape_string(end_timestamp)
        self.db_conn.query("SELECT `timestamp`,`stat_names`.`name` AS `stat`,`value` FROM `stats` JOIN `stat_names` ON `stats`.`stat` = `stat_names`.`id` WHERE (`timestamp` BETWEEN '%s' AND '%s') AND (`run` = '%s') ORDER BY `timestamp` ASC" % (start_timestamp, end_timestamp, run) )
        result = self.db_conn.use_result()
        rows = result.fetch_row(maxrows=0) # Fetch all rows
        last_ts = -1
        stats = []
        current_sample = {}
        for row in rows:
            if row[0] != last_ts and len(current_sample) > 0:
                stats.append(current_sample) # Store the previous sample
                current_sample = {}
            current_sample[row[1]] = float(row[2])
            last_ts = row[0]

        if len(current_sample) > 0:
            stats.append(current_sample) # Store the last sample

        return stats

