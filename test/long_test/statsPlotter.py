#!/usr/bin/python
import sys, os, signal, math
import mysql
from datetime import datetime
from subprocess import Popen, PIPE
from optparse import OptionParser
from retester import send_email, do_test

def simple_plotter(name, multiplier = 1):
    return lambda (old, new): new[name]*multiplier if new[name] else None

def differential_plotter(name):
    return lambda (old, new): (new[name]-old[name])/(new['uptime']-old['uptime']) if new[name] and old[name] else None

def two_stats_diff_plotter(stat1, stat2):
    return lambda (old, new): new[stat1]-new[stat2] if new[stat1] and new[stat2] else None

def two_stats_ratio_plotter(dividend, divisor):
    return lambda (old, new): new[dividend]/new[divisor] if new[dividend] and new[divisor] and new[divisor] != 0 else None

class Plot(object)
    single_plot_height = 128

    def __init__(self, stats, name, plot_style, plotter):
        self.name = name
        self.plot_style = plot_style
        self.plotter = plotter
        self.plot = self.generate_plot(stats)
        
    def get_plot(self)
        return self.plot

    def generate_plots(self, all_stats):
        zipped_stats = zip(all_stats, all_stats[1:])
        plot_instructions = ['-k', self.name, self.plot_style]

        tplot = Popen(['tplot',
            '-if', '-',
            '-o', '/dev/stdout',
            '-of', 'png',
            '-tf', 'num',
            '-or', '1024x%d' % (DBPlot.single_plot_height),
            ] + plot_instructions, stdin=PIPE, stdout=PIPE)

        for stats_pair in zipped_stats:
            val = self.plotter(stats_pair)
            if val:
                ts = stats_pair[1]['uptime']
                print >>tplot.stdin, "%f =%s %f" % (ts, self.name, val)

        tplot.stdin.close()
        return [tplot.stdout.read()]

class DBPlot(Plot):
    def __init__(self, start_timestamp, end_timestamp, name, plot_style, plotter):
        self.db_conn = _mysql.connect("newton", "longtest", "rethinkdb2010", "longtest") # TODO
        stats = self.load_stats_from_db(start_timestamp, end_timestamp)
        self.db_conn.close()
        
        Plot.__init__(self, stats, name, plot_style, plotter)
        
    def load_stats_from_db(self, start_timestamp, end_timestamp):
        start_timestamp = self.db_conn.escape_string(start_timestamp)
        end_timestamp = self.db_conn.escape_string(end_timestamp)
        self.db_conn.query("SELECT `timestamp`,`stat`,`value` FROM `stats` WHERE `timestamp` BETWEEN '%s' AND '%s' ORDER BY `timestamp` ASC" % (start_timestamp, end_timestamp) )
        result = db_conn.use_result()
        rows = result.fetch_row(maxrows=0) # Fetch all rows
        last_ts = -1
        stats = []
        current_sample = {}
        for row in rows:
            if row[0] != last_ts and len(current_sample) > 0:
                stats.append(current_sample) # Store the previous sample
                current_sample = {}
            current_sample[row[1]] = row[2]
            last_ts = row[0]
            
        if len(current_sample) > 0:
            stats.append(current_sample) # Store the last sample
            
        return stats

        
    

