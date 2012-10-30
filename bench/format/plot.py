# Copyright 2010-2012 RethinkDB, all rights reserved.
import os, sys
import numpy as np
import matplotlib as mpl
mpl.use('Agg') # can't use tk since we don't have X11
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
import matplotlib.font_manager as fm
import matplotlib.gridspec as gridspec
from matplotlib.ticker import FuncFormatter
from mpl_toolkits.mplot3d import Axes3D
from matplotlib.collections import PolyCollection
from colors import *
import json
import time
from line import *
from statlib import stats
import math

FONT_FILE = os.path.abspath(os.path.join(os.path.dirname(__file__), 'aurulent_sans_regular.ttf'))

def safe_div(x, y):
    if y == 0:
        return x
    else:
        return x / y

def cull_outliers(data, n_sigma):
    mean = stats.mean(map(lambda x: x, data))
    sigma  = stats.stdev(map(lambda x: x, data))
    return filter(lambda x: abs(x - mean) < n_sigma * sigma, data)

def clip(data, min, max):
    return filter(lambda x: min<x<max, data)

def normalize(array):
    denom = max(map(lambda x: abs(x), array))
    if denom == 0:
        return array
    else:
        return map(lambda x: float(x) / denom, array)

class TimeSeries(list):
    def __init__(self, units):
        self.units = units

class Scatter():
    def __init__(self, list_of_tuples, xnames = None):
        self.data = list_of_tuples
        self.names = xnames
    def __str__(self):
        return "self.data: " + str(self.data) + "\nself.names: " + str(self.names)+"\n\n"

class default_empty_timeseries_dict(dict):
    units_line = line("(\w+)\[(\w+)\]", [('key', 's'), ('units', 's')])
    def __setitem__(self, key, item):
        m = self.units_line.parse_line(key)
        if m:
            key = m['key']
            units = m['units']
        dict.__setitem__(self, key, item)

    def __getitem__(self, key):
        m = self.units_line.parse_line(key)
        if m:
            key = m['key']
            units = m['units']
        else:
            units = ''
        if key in self:
            return self.get(key)
        else:
            return TimeSeries(units)

    def copy(self):
        copy = default_empty_timeseries_dict()
        copy.update(self)
        return copy

    # Hack to enforce a specific ordering of RethinkDB and competitors
    def iteritems(self):
        sorted_keys = self.keys()
        sorted_keys.sort()
        sorted_keys.reverse()
        result = []
        # Put RethinkDB to the beginning
        if 'RethinkDB' in self:
            result.append(('RethinkDB', self['RethinkDB']))
        for x in sorted_keys:
            if x != 'RethinkDB':
                result.append((x, self[x]))

        return iter(result)


class default_empty_plot_dict(dict):
    def __getitem__(self, key):
        if key in self:
            return self.get(key)
        else:
            return Scatter([])
    def copy(self):
        copy = default_empty_plot_dict()
        copy.update(self)
        return copy

class TimeSeriesCollection():
    def __init__(self):
        self.data = default_empty_timeseries_dict()

    def __str__(self):
        return "TimeSeriesCollection data: " + str(self.data) +"\n"

    def read(self, file_name):
        try:
            data = open(file_name).readlines()
        except IOError:
            print 'Missing file: %s data from it will not be reported' % file_name
            return self
        try:
            self.data = self.parse(data)
        except AssertionError:
            print 'Malformed data from %s' % file_name
            return self
        try:
            self.process()
        except AssertionError:
            print "Processing failed on %s" % file_name
            return self

        return self #this just lets you do initialization in one line

    def copy(self):
        copy = self.__class__()
        copy.data = self.data.copy()
        return copy

    def __add__(self, other):
        res = self.copy()
        for val in other.data.iteritems():
            assert not val[0] in res.data
            res.data[val[0]] = val[1]
        return res

#limit the data to just the keys in keys
    def select(self, keys):
        copy = self.copy()
        for key in copy.data.keys():
            if not key in keys:
                copy.data.pop(key)

        return copy

    def drop(self, keys):
        copy = self.copy()
        for key in copy.data.keys():
            if key in keys:
                copy.data.pop(key)

        return copy

    def remap(self, orig_name, new_name):
        copy = self.drop(orig_name)
        copy.data[new_name] = self.data[orig_name]
        return copy

    def parse(self, data):
        pass

#do post processing things on the data (ratios and derivatives and stuff)
    def process(self):
        pass

    def shorten(self):
        if len(self.data.values()[0]) > 1500:
            for key in self.data.keys():
                self.derive(key, (key,), drop_points)

    def smooth_curves(self):
        for key in self.data.keys():
            self.derive(key, (key,), smooth)

    def json(self, out_fname, meta_data):
        top_level = {}
        top_level['date'] = time.asctime() 
        top_level['meta'] = meta_data
        top_level['data'] = {}
        top_level['data']['rethinkdb'] = {}
        for series in self.data.iteritems():
            top_level['data']['rethinkdb'][series[0]] = {}
            top_level['data']['rethinkdb'][series[0]]['data'] = map(lambda x: list(x), zip(range(len(series[1])), series[1]))
            top_level['data']['rethinkdb'][series[0]]['unit'] = series[1].units

        f = open(out_fname + '.js', 'w')
        print >>f, json.dumps(top_level)
        f.close()

    def histogram(self, out_fname, large = False):
        assert self.data

        # Hacky workaround in the case that a run had no data.
        for x in self.data.values():
            if len(x) == 0: x.extend([0,0])

        if not large:
            font = fm.FontProperties(family=['sans-serif'],size='small',fname=FONT_FILE)
            mpl.rcParams['xtick.major.pad'] = 4
            mpl.rcParams['ytick.major.pad'] = 4
            mpl.rcParams['lines.linewidth'] = 1
        else:
            font = fm.FontProperties(family=['sans-serif'],size=36,fname=FONT_FILE)
            mpl.rcParams['xtick.major.pad'] = 20
            mpl.rcParams['ytick.major.pad'] = 20
            mpl.rcParams['lines.linewidth'] = 5

        fig = plt.figure()
        # Set the margins for the plot to ensure a minimum of whitespace
        ax = plt.axes([0.12,0.12,0.85,0.85])

        data = map(lambda x: x[1], self.data.iteritems())
        mean = stats.mean(map(lambda x: x, reduce(lambda x, y: x + y, data)))
        stdev = stats.stdev(map(lambda x: x, reduce(lambda x, y: x + y, data)))
        labels = []
        hists = []
        for series, color in zip(self.data.iteritems(), colors):
            clipped_data = clip(series[1], 0, 3 * mean)
            if clipped_data:
                _, _, foo = ax.hist(clipped_data, bins=200, histtype='bar', facecolor = color, alpha = .5, label = series[0])
                hists.append(foo)
                labels.append(series[0])
            else:
                print "Tried to make a histogram of a series of size 0"

        for tick in ax.xaxis.get_major_ticks():
            tick.label1.set_fontproperties(font)
        for tick in ax.yaxis.get_major_ticks():
            tick.label1.set_fontproperties(font)

        ax.set_ylabel('Frequency', fontproperties = font)
        ax.set_xlabel('Latency (microseconds)', fontproperties = font) #simply should not be hardcoded but we want nice pictures now
        ax.grid(True)
        # Dirty hack to get around legend miscoloring: drop all the hists generated into the legend one by one
        if hists:
            plt.legend(map(lambda x: x[0], hists), labels, loc=1, prop = font)
        
        if not large:
            fig.set_size_inches(5,3.7)
            fig.set_dpi(90)
            plt.savefig(out_fname, bbox_inches="tight")
        else:
            ax.yaxis.LABELPAD = 40
            ax.xaxis.LABELPAD = 40
            fig.set_size_inches(20,14.8)
            fig.set_dpi(300)
            plt.savefig(out_fname, bbox_inches="tight")

    def plot(self, out_fname, large = False, normalize = False):
        assert self.data

        # Hacky workaround in the case that no run had any data.
        if all(len(x) == 0 for x in self.data.values()):
            self.data['Error: No data'] = [0]

        queries_formatter = FuncFormatter(lambda x, pos: '%1.fk' % (x*1e-3))
        if not large:
            font = fm.FontProperties(family=['sans-serif'],size='small',fname=FONT_FILE)
            mpl.rcParams['xtick.major.pad'] = 4
            mpl.rcParams['ytick.major.pad'] = 4
            mpl.rcParams['lines.linewidth'] = 1
        else:
            font = fm.FontProperties(family=['sans-serif'],size=36,fname=FONT_FILE)
            mpl.rcParams['xtick.major.pad'] = 20
            mpl.rcParams['ytick.major.pad'] = 20
            mpl.rcParams['lines.linewidth'] = 5
        
        fig = plt.figure()
        # Set the margins for the plot to ensure a minimum of whitespace
        ax = plt.axes([0.13,0.12,0.85,0.85])
        labels = []
        color_index = 0
        for series, series_data in self.data.iteritems():
            if normalize:
                data_to_use = normalize(series_data)
            else:
                data_to_use = series_data
            #labels.append((ax.plot(range(len(series_data)), data_to_use, colors[color_index]), series, 'bo'))
            labels.append((ax.plot(range(len(series_data)), data_to_use, colors[color_index], linestyle="None", marker="."), series, 'bo'))

            color_index += 1
         
        for tick in ax.xaxis.get_major_ticks():
            tick.label1.set_fontproperties(font)
        for tick in ax.yaxis.get_major_ticks():
            tick.label1.set_fontproperties(font)

        ax.yaxis.set_major_formatter(queries_formatter)
        ax.set_ylabel('Queries', fontproperties = font)
        ax.set_xlabel('Time (seconds)', fontproperties = font)

        ax.set_xlim(0, len(self.data[self.data.keys()[0]]) - 1)
        if normalize:
            ax.set_ylim(0, 1.2)
        else:
            reduced_list = reduce(lambda x,y: x + y, self.data.values())
            max_relevant_value = max(reduced_list[(len(reduced_list) / 10):]) # Ignore first 10% of the data for ylim calculation, as they might contain high peaks
            ax.set_ylim(0, 1.2 * max_relevant_value)
        ax.grid(True)
        plt.legend(tuple(map(lambda x: x[0], labels)), tuple(map(lambda x: x[1], labels)), loc=1, prop = font)

        if not large:
            fig.set_size_inches(5,3.7)
            fig.set_dpi(90)
            plt.savefig(out_fname, bbox_inches="tight")
        else:
            ax.yaxis.LABELPAD = 40
            ax.xaxis.LABELPAD = 40
            fig.set_size_inches(20,14.8)
            fig.set_dpi(300)
            plt.savefig(out_fname, bbox_inches="tight")
            
    # This is from http://code.activestate.com/recipes/511478/
    def percentile(self, N, percent, key=lambda x:x):
        """
        Find the percentile of a list of values.

        @parameter N - is a list of values. Note N MUST BE already sorted.
        @parameter percent - a float value from 0.0 to 1.0.
        @parameter key - optional key function to compute value from each element of N.

        @return - the percentile of the values
        """
        if not N:
            return None
        k = (len(N)-1) * percent
        f = math.floor(k)
        c = math.ceil(k)
        if f == c:
            return key(N[int(k)])
        d0 = key(N[int(f)]) * (k-f)
        d1 = key(N[int(c)]) * (c-k)
        return d0+d1

    def stats(self):
        res = {}
        for val in self.data.iteritems():
            stat_report = {}
	    full_series = map(lambda x: x, val[1])
	    full_series = full_series[(len(full_series) / 4):] # Cut off first quarter to get more reliable data
	    full_series_sorted = full_series
	    full_series_sorted.sort()
	    steady_series = full_series[int(len(full_series) * 0.7):]
            stat_report['mean'] = stats.mean(full_series)
            try:
                stat_report['stdev'] = stats.stdev(full_series)
            except ZeroDivisionError:
                stat_report['stdev'] = 0
            stat_report['upper_0.1_percentile'] = self.percentile(full_series_sorted, 0.999)
            stat_report['lower_0.1_percentile'] = self.percentile(full_series_sorted, 0.001)
            stat_report['upper_1_percentile'] = self.percentile(full_series_sorted, 0.99)
            stat_report['lower_1_percentile'] = self.percentile(full_series_sorted, 0.01)
            stat_report['upper_5_percentile'] = self.percentile(full_series_sorted, 0.95)
            stat_report['lower_5_percentile'] = self.percentile(full_series_sorted, 0.05)
            stat_report['steady_mean'] = stats.mean(steady_series)
            try:
                stat_report['steady_stdev'] = stats.stdev(full_series)
            except ZeroDivisionError:
                stat_report['steady_stdev'] = 0
	    res[val[0]] = stat_report

        return res

#function : (serieses)*len(arg_keys) -> series
#TODO this should return a copy with the changes
    def derive(self, name, arg_keys, function):
        args = []
        for key in arg_keys:
            if not key in self.data:
                return self
            args.append(self.data[key])

        self.data[name] = function(tuple(args))
        return self

class ScatterCollection():
    def __init__(self, TimeSeriesCollections, xlabel = None, ylabel = None):
        self.data = default_empty_plot_dict()
        self.xlabel = xlabel
        self.ylabel = ylabel
        self.data.update(TimeSeriesCollections)

    def plot(self, out_fname, large = False, normalize = False):
        assert self.data

        max_x_value = 0
        max_y_value = 0

        queries_formatter = FuncFormatter(lambda x, pos: '%1.fk' % (x*1e-3))
        if not large:
            font = fm.FontProperties(family=['sans-serif'],size='small',fname=FONT_FILE)
            mpl.rcParams['xtick.major.pad'] = 4
            mpl.rcParams['ytick.major.pad'] = 4
            mpl.rcParams['lines.linewidth'] = 1
        else:
            font = fm.FontProperties(family=['sans-serif'],size=36,fname=FONT_FILE)
            mpl.rcParams['xtick.major.pad'] = 20
            mpl.rcParams['ytick.major.pad'] = 20
            mpl.rcParams['lines.linewidth'] = 5
        fig = plt.figure()
        # Set the margins for the plot to ensure a minimum of whitespace
        ax = plt.axes([0.13,0.12,0.85,0.85])
        labels = []
        color_index = 0
        for series,series_data in self.data.iteritems():
            x_values = []
            y_values = []
            for (x_value, y_value) in series_data:
                max_x_value = max(max_x_value, x_value)
                max_y_value = max(max_y_value, y_value)
                x_values.append(x_value)
                y_values.append(y_value)
            if normalize:
                y_values = normalize(y_values)

            labels.append((ax.plot(x_values, y_values, colors[color_index]), series))
            color_index += 1
         
        for tick in ax.xaxis.get_major_ticks():
            tick.label1.set_fontproperties(font)
        for tick in ax.yaxis.get_major_ticks():
            tick.label1.set_fontproperties(font)

        ax.yaxis.set_major_formatter(queries_formatter)
        if self.ylabel:
            ax.set_ylabel(self.ylabel, fontproperties = font)
        else:
            ax.set_ylabel('Queries', fontproperties = font)

        if self.xlabel:
            ax.set_xlabel(self.xlabel, fontproperties = font)
        else:
            ax.set_xlabel('N/A', fontproperties = font)

        ax.set_xlim(0, max_x_value)
        if normalize:
            ax.set_ylim(0, 1.2)
        else:
            ax.set_ylim(0, 1.2 * max_y_value)
        ax.grid(True)
        plt.legend(tuple(map(lambda x: x[0], labels)), tuple(map(lambda x: x[1], labels)), loc=1, prop = font)
        fig.set_size_inches(5,3.7)

        if not large:
            fig.set_size_inches(5,3.7)
            fig.set_dpi(90)
            plt.savefig(out_fname, bbox_inches="tight")
        else:
            ax.yaxis.LABELPAD = 40
            ax.xaxis.LABELPAD = 40
            fig.set_size_inches(20,14.8)
            fig.set_dpi(300)
            plt.savefig(out_fname, bbox_inches="tight")

class SubplotCollection():
    def __init__(self, TimeSeriesCollections, xlabel = None, ylabel = None):
        self.data = default_empty_plot_dict()
        self.xlabel = xlabel
        self.ylabel = ylabel
        self.data.update(TimeSeriesCollections)

    # Plots a single subplot for use in the whole figure
    def plot_subplot(self, ax, subplot, queries_formatter, font):
        max_y_value = 0

        # Set the margins for the plot to ensure a minimum of whitespace
        labels = []
        color_index = 0
        for series,series_data in subplot.data.iteritems():
            y_values = []
            for y_value in series_data:
            #     max_y_value = max(max_y_value, y_value)
                y_values.append(y_value)
            #if normalize:
            #    y_values = normalize(y_values)

            labels.append((ax.plot(range(len(series_data)), y_values, colors[color_index]), series))
            color_index += 1
         
        # Hide the x and y tick labels, but keep the actual ticks
        ax.set_xticklabels([])
        ax.set_yticklabels([])
        #for tick in ax.xaxis.get_major_ticks():
        #    tick.label1.set_fontproperties(font)
        #for tick in ax.yaxis.get_major_ticks():
        #   #tick.label1.set_fontproperties(font)

       # ax.yaxis.set_major_formatter(queries_formatter)

        ax.set_xlim(0, len(subplot.data[subplot.data.keys()[0]]) - 1)
        #if normalize:
        #    ax.set_ylim(0, 1.0)
        #else:
        #    ax.set_ylim(0, max_y_value)
        ax.grid(True)

    # Plots a given set of subplots onto a figure
    def plot(self, out_fname, large = False, normalize = False):
        assert self.data

        queries_formatter = FuncFormatter(lambda x, pos: '%1.fk' % (x*1e-3))
        if not large:
            font = fm.FontProperties(family=['sans-serif'],size='xx-small',fname=FONT_FILE)
            ax_label_font = fm.FontProperties(family=['sans-serif'],size='small',fname=FONT_FILE)
#            mpl.rcParams['xtick.major.pad'] = 4
#            mpl.rcParams['ytick.major.pad'] = 4
            mpl.rcParams['lines.linewidth'] = 1
        else:
            font = fm.FontProperties(family=['sans-serif'],size='medium',fname=FONT_FILE)
            ax_label_font = fm.FontProperties(family=['sans-serif'],size=36,fname=FONT_FILE)
            mpl.rcParams['xtick.major.pad'] = 20
            mpl.rcParams['ytick.major.pad'] = 20
            mpl.rcParams['lines.linewidth'] = 3

        # data has a list of dictionaries representing subplots, so the number of subplots is the length of the list
        num_subplots = len(self.data)
        subplot_dim = int(math.ceil(math.sqrt(num_subplots)))

        fig = plt.figure()

        # gs_wrapper contains the x and y axis labels, and the subplots' GridSpec
        gs_wrapper = gridspec.GridSpec(2, 2, wspace = 0.05, hspace = 0.05, width_ratios = [1,100], height_ratios = [100,1], left=0.01, bottom=0.01)

        # x_title_ax and y_title_ax are empty plots to ensure we always have a common x and y axis label for all subplots
        # We have to hide the grid, ticks, and border, so only the labels remain.
        x_title_ax = plt.subplot(gs_wrapper[1,:])
        x_title_ax.set_xlabel('Times (seconds)', fontproperties = ax_label_font)
        y_title_ax = plt.subplot(gs_wrapper[0,0])
        y_title_ax.set_ylabel('Queries', fontproperties = ax_label_font) 
        for title_ax in [x_title_ax, y_title_ax]:
            title_ax.grid(False)
            title_ax.set_xticks([])
            title_ax.set_yticks([])
            title_ax.set_frame_on(False)
            for tick in title_ax.get_xticklabels():
                tick.set_visible(False)
            for tick in title_ax.get_yticklabels():
                tick.set_visible(False)

        # This GridSpec defines the layout of the subplots
        gs = gridspec.GridSpecFromSubplotSpec(subplot_dim, subplot_dim, subplot_spec=gs_wrapper[0,1])
#        gs = gridspec.GridSpec(subplot_dim, subplot_dim)

	    # Sort the data keys so that the subplots display in order.
        sorted_data_keys = self.data.keys()
        sorted_data_keys.sort()

        for i in range(subplot_dim):
            for j in range(subplot_dim):
                subplot_num = i * subplot_dim + j
                if subplot_num < num_subplots:
                    self.plot_subplot(plt.subplot(gs[i,j]), self.data[sorted_data_keys[subplot_num]], queries_formatter, font)
        
        if not large:
            fig.set_size_inches(5,3.7)
            fig.set_dpi(90)
            plt.savefig(out_fname, bbox_inches="tight")
        else:
            #ax.yaxis.LABELPAD = 40
            #ax.xaxis.LABELPAD = 40
            fig.set_size_inches(20,14.8)
            fig.set_dpi(300)
            plt.savefig(out_fname, bbox_inches="tight")

class TimeSeriesMeans():
    def __init__(self, TimeSeriesCollections):
        TS = reduce(lambda x,y: x + y, TimeSeriesCollections)
        series_means = means(TS.data.values())
        names = {}
        for i,key in zip(range(len(TS.data.keys())), TS.data.keys()):
            names[i] = key

        self.scatter = Scatter(zip(range(len(series_means)), series_means), names)
        self.TimeSeriesCollections = TimeSeriesCollections
    def __str__(self):
        return self.scatter.__str__() + self.TimeSeriesCollections.__str__()

#A few useful derivation functions
#take discret derivative of a series (shorter by 1)
def differentiate(series):
#series will be a tuple
    series = series[0]
    res = []
    for f_t, f_t_plus_one in zip(series[:len(series) - 1], series[1:]):
        res.append(f_t_plus_one - f_t)

    return res

def difference(serieses):
    assert serieses[0].units == serieses[1].units
    res = TimeSeries(serieses[0].units)
    for x,y in zip(serieses[0], serieses[1]):
        res.append(x - y)

    return res

def slide(serieses):
    res = TimeSeries('seconds')
    for i in range(min(map(lambda x: len(x), serieses))):
        for j in range(i, len(serieses[1])):
            if serieses[0][i] <= serieses[1][j]:
                res += [j - i]
                break
    return res

def ratio(serieses):
    res = TimeSeries(serieses[0].units + '/' + serieses[1].units)
    for x,y in zip(serieses[0], serieses[1]):
        if y == 0:
            res += [None]
        else:
            res += [float(x)/float(y)]
    return res

#report the means of a list of runs
def means(serieses):
    res = []
    for series in serieses:
        try:
            res.append(stats.mean(map(lambda x: x, series)))
        except ZeroDivisionError:
            res.append(0.0)
    return res

def drop_points(serieses):
    res = TimeSeries([])
    res.units = serieses[0].units
    for i in range(0, len(serieses[0]), max(len(serieses[0]) / 1000, 1)):
        res.append(serieses[0][i])
    return res

def smooth(serieses):
    res = TimeSeries([])
    res.units = serieses[0].units
    for i in range(len(serieses[0])):
        index_range = range(max(i - 15, 0), min(i + 15, len(serieses[0]) - 1))
        val = 0
        for j in index_range:
            if serieses[0][j]:
                val += serieses[0][j] / len(index_range)

        res += [val]

    return res

class IOStat(TimeSeriesCollection):
    file_hdr_line   = line("Linux.*", [])
    avg_cpu_hdr_line= line("^avg-cpu:  %user   %nice %system %iowait  %steal   %idle$", [])
    avg_cpu_line    = line("^" + "\s+([\d\.]+)" * 6 + "$", [('user', 'f'), ('nice', 'f'), ('system', 'f'), ('iowait', 'f'),  ('steal', 'f'),   ('idle', 'f')])
    dev_hdr_line    = line("^Device:            tps   Blk_read/s   Blk_wrtn/s   Blk_read   Blk_wrtn$", [])
    dev_line        = line("^(\w+)\s+([\d\.]+)\s+([\d\.]+)\s+([\d\.]+)\s+(\d+)\s+(\d+)$", [('device', 's'), ('tps', 'f'), (' Blk_read', 'f'), (' Blk_wrtn', 'f'), (' Blk_read', 'd'), (' Blk_wrtn', 'd')])

    def parse(self, data):
        res = default_empty_timeseries_dict()
        data.reverse()
        m = until(self.file_hdr_line, data)
        assert m != False
        while True:
            m = until(self.avg_cpu_hdr_line, data)
            if m == False:
                break

            m = take(self.avg_cpu_line, data)
            assert m
            for val in m.iteritems():
                res['cpu_' + val[0]] += [val[1]]

            m = until(self.dev_hdr_line, data)
            assert m != False

            m = take_while([self.dev_line], data)
            for device in m:
                dev_name = device.pop('device')
                for val in device.iteritems():
                    res['dev:' + dev_name + '_' + val[0]] += [val[1]]

        return res

class VMStat(TimeSeriesCollection):
    file_hdr_line   = line("^procs -----------memory---------- ---swap-- -----io---- -system-- ----cpu----$", [])
    stats_hdr_line  = line("^ r  b   swpd   free   buff  cache   si   so    bi    bo   in   cs us sy id wa$", [])
    stats_line      = line("\s+(\d+)" * 16, [('r', 'd'),  ('b', 'd'),   ('swpd', 'd'),   ('free', 'd'),   ('buff', 'd'),  ('cache', 'd'),   ('si', 'd'),   ('so', 'd'),    ('bi', 'd'),    ('bo', 'd'),   ('in', 'd'),   ('cs', 'd'), ('us', 'd'), ('sy', 'd'), ('id', 'd'), ('wa', 'd')])

    def parse(self, data):
        res = default_empty_timeseries_dict()
        data.reverse()
        while True:
            m = until(self.file_hdr_line, data)
            if m == False:
                break
            m = take(self.stats_hdr_line, data)
            assert m != False
            m = take_while([self.stats_line], data)
            for stat_line in m:
                for val in stat_line.iteritems():
                    res[val[0]]+= [val[1]]
        return res

class Latency(TimeSeriesCollection):
    line = line("(\d+)\s+([\d.]+)\n", [('tick', 'd'), ('latency', 'f')])

    def parse(self, data):
        res = default_empty_timeseries_dict()
        for line in data:
            res['latency'] += [self.line.parse_line(line)['latency']]
        return res

class QPS(TimeSeriesCollection):
    line = line("(\d+)\s+([\d]+)\n", [('tick', 'd'), ('qps', 'f')])

    def parse(self, data):
        res = default_empty_timeseries_dict()
        for line in data:
            res['qps'] += [self.line.parse_line(line)['qps']]
        return res

class RDBStats(TimeSeriesCollection):
    int_line = line("^STAT\s+([\w\[\]]+)\s+(\d+|-)\r$", [('name', 's'), ('value', 'd')])
    flt_line = line("^STAT\s+([\w\[\]]+)\s+([\d\.]+|-)\r$", [('name', 's'), ('value', 'f')])
    str_line = line("^STAT\s+([\w\[\]]+)\s+(.+|-)\r$", [('name', 's'), ('value', 's')])
    end_line = line("END", [])
    
    def parse(self, data):
        res = default_empty_timeseries_dict()
        data.reverse()
        
        while True:
            m = take_while([self.int_line, self.flt_line, self.str_line], data)
            if not m:
                break
            for match in m:
                res[match['name']] += [match['value']]

            m = take(self.end_line, data)
	    if m == False: # Incomplete stats, might happen if monitor was killes while retrieving stats response
                break

            if res:
                lens = map(lambda x: len(x[1]), res.iteritems())
                assert max(lens) == min(lens)
        return res

    def process(self):
        differences = [('io_reads_completed', 'io_reads_started'), 
                       ('io_writes_started', 'io_writes_completed')]
        ratios = [('conns_reading_total', 'conns_total'),
                  ('conns_writing_total', 'conns_total'),
                  ('blocks_dirty', 'blocks_total'),
                  ('blocks_in_memory', 'blocks_total'),
                  ('serializer_old_garbage_blocks',  'serializer_old_total_blocks')]

        slides = [('io_writes_started', 'io_writes_completed')]
#        differences = [('io_reads_completed', 'io_reads_started'), 
#                       ('io_writes_started', 'io_writes_completed'), 
#                       ('transactions_started', 'transactions_ready'), 
#                       ('transactions_ready', 'transactions_completed'),
#                       ('bufs_acquired', 'bufs_ready'),
#                       ('bufs_ready', 'bufs_released')]
#        ratios = [('conns_in_btree_incomplete', 'conns_total'),
#                  ('conns_in_outstanding_data', 'conns_total'),
#                  ('conns_in_socket_connected', 'conns_total'),
#                  ('conns_in_socket_recv_incomplete', 'conns_total'),
#                  ('conns_in_socket_send_incomplete', 'conns_total'),
#                  ('blocks_dirty', 'blocks_total'),
#                  ('blocks_in_memory', 'blocks_total'),
#                  ('serializer_old_garbage_blocks',  'serializer_old_total_blocks')]
#
#        slides = [('flushes_started', 'flushes_acquired_lock'),
#                  ('flushes_acquired_lock', 'flushes_completed'),
#                  ('io_writes_started', 'io_writes_completed')]
        keys_to_drop = set()
        for dif in differences:
            self.derive(dif[0] + ' - ' + dif[1], dif, difference)
            keys_to_drop.add(dif[0])
            keys_to_drop.add(dif[1])

        for rat in ratios:
            self.derive(rat[0] + ' / ' + rat[1], rat, ratio)
            keys_to_drop.add(rat[0])

        self.remap('serializer_old_garbage_blocks / serializer_old_total_blocks', 'garbage_ratio')

        for s in slides:
            self.derive('slide(' + s[0] + ', ' + s[1] + ')', s, slide)
            keys_to_drop.add(s[0])
            keys_to_drop.add(s[1])

        self.drop(keys_to_drop)

ncolumns = 37 #TODO man this is awful
class RunStats(TimeSeriesCollection):
    name_line = line("^" + "([\#\d_]+)[\t\n]+"* ncolumns, [('col%d' % i, 's') for i in range(ncolumns)])
    data_line = line("^" + "(\d+)[\t\n]+"* ncolumns, [('col%d' % i, 'd') for i in range(ncolumns)])

    def parse(self, data):
        res = default_empty_timeseries_dict()

        data.reverse()

        m = take(self.name_line, data)
        assert m
        col_names = m

        m = take_while([self.data_line], data)
        for line in m:
            for col in line.iteritems():
                res[col_names[col[0]]] += [col[1]]

        return res
