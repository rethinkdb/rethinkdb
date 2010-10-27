import numpy as np
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
from mpl_toolkits.mplot3d import Axes3D
from matplotlib.collections import PolyCollection
import re
from colors import *

def normalize(array):
    denom = max(map(lambda x: abs(x), array))
    if denom == 0:
        return array
    else:
        return map(lambda x: float(x) / denom, array)

class default_empty_dict(dict):
    def __getitem__(self, key):
        if key in self:
            return self.get(key)
        else:
            return []
    def copy(self):
        copy = default_empty_dict()
        copy.update(self)
        return copy

#TODO this code is copy pasted from oprofile.py. They should be moved out into a seperate library
class line():
    regex = ""
    fields = [] #tuples of form name, type
    def __init__(self, _regex, _fields):
        self.regex = _regex
        self.fields = _fields

    def __repr__(self):
        return self.regex

    def parse_line(self, line):
        matches = re.match(self.regex, line)
        if matches:
            result = {}
            for field, groupi in zip(self.fields, range(1, len(self.fields) + 1)):
                if (field[1] == 'd'):
                    val = int(matches.group(groupi))
                elif (field[1] == 'f'):
                    val = float(matches.group(groupi))
                elif (field[1] == 's'):
                    val = matches.group(groupi)
                else:
                    assert 0
                result[field[0]] = val
            return result
        else:
            return False

def take(line, data):
    if len(data) == 0:
        return False
    matches = line.parse_line(data[len(data) - 1])
    data.pop()
    return matches

def take_maybe(line, data):
    if len(data) == 0:
        return False
    matches = line.parse_line(data[len(data) - 1])
    if matches != False: data.pop()
    return matches

#look through an array of data until you get a match (or run out of data)
def until(line, data):
    while len(data) > 0:
        matches = line.parse_line(data[len(data) - 1])
        data.pop()
        if matches != False:
            return matches
    return False

#iterate through lines while they match (and there's data)
def read_while(lines, data):
    res = []
    while len(data) > 0:
        for line in lines:
            m = line.parse_line(data[len(data) - 1])
            if m:
                break
        if m:
            res.append(m)
            data.pop()
        else:
            break
    return res

class TimeSeries():
    def __init__(self):
        self.data = default_empty_dict()

    def read(self, file_name):
        self.data = self.parse_file(file_name)
        self.process()
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
        for key in self.data.keys():
            if not key in keys:
                self.data.pop(keys)

    def drop(self, keys):
        for key in self.data.keys():
            if key in keys:
                self.data.pop(keys)

    def parse_file(self, file_name):
        pass

#do post processing things on the data (ratios and derivatives and stuff)
    def process(self):
        pass

    def histogram(self, out_fname):
        assert self.data
        for series in self.data.iteritems():
            fig = plt.figure()
            ax = fig.add_subplot(111)
            ax.hist(series[1], 40, range=(0, (sum(series[1]) / len(series[1])) * 2), facecolor='green', alpha=1.0)
            ax.set_xlabel(series[0])
            ax.set_ylabel('Count')
            ax.set_xlim(0, (sum(series[1]) / len(series[1])) * 2)
            ax.set_ylim(0, len(series[1]) / 10)
            ax.grid(True)
            plt.savefig(out_fname + series[0])

    def plot(self, out_fname):
        assert self.data
        fig = plt.figure()
        ax = fig.add_subplot(111)
        labels = []
        color_index = 0
        for series in self.data.iteritems():
            if len(self.data) > 1:
                data_to_use = normalize(series[1])
            else:
                data_to_use = series[1]
            labels.append((ax.plot(range(len(series[1])), data_to_use, colors[color_index]), series[0]))
            color_index += 1

        plt.figlegend(tuple(map(lambda x: x[0], labels)), tuple(map(lambda x: x[1], labels)), 'upper right', shadow=True)
        ax.set_xlabel('Time (seconds)')
        ax.set_xlim(0, len(self.data[self.data.keys()[0]]) - 1)
        if len(self.data) > 1:
            ax.set_ylim(0, 1.0)
        else:
            ax.set_ylim(0, max(self.data[self.data.keys()[0]]))
        ax.grid(True)
        plt.savefig(out_fname, dpi=300)

#function : (serieses)/len(arg_names) -> series
    def derive(self, name, arg_keys, function):
        args = []
        for key in arg_keys:
            args.append(self.data[key])

        self.data[name] = function(tuple(args))

def multi_plot(timeseries, out_fname):
    fig = plt.figure()
    ax = fig.add_subplot(111)
    verts = []

    xs = range(min(map(lambda x: min(map(lambda y: len(y[1]), x.data.iteritems())), timeseries)))

    for z in range(len(timeseries)):
        for series in timeseries[z].data.iteritems():
            verts.append(zip(xs, series[1]))

    poly = PolyCollection(verts, facecolors = colors[0:len(verts)])
    poly.set_alpha(0.7)
    ax.add_collection3d(poly, range(len(timeseries)), zdir='y')

    ax.set_xlabel('X')
    ax.set_xlim3d(0, len(xs))
    ax.set_ylabel('Y')
    ax.set_ylim3d(-1, len(timeseries))
    ax.set_zlabel('Z')
    ax.set_zlim3d(0, max(map(lambda x: max(x), timeseries)))
    plt.savefig(out_fname, dpi=300)

#take discret derivative of a series (shortens series by 1)
def differentiate(series):
#series will be a tuple
    series = series[0]
    res = []
    for f_t, f_t_plus_one in zip(series[:len(series) - 1], series[1:]):
        res.append(f_t_plus_one - f_t)

    return res

def difference(serieses):
    res = []
    for x,y in zip(serieses[0], serieses[1]):
        res.append(x - y)

    return res

class IOStat(TimeSeries):
    file_hdr_line   = line("Linux.*", [])
    avg_cpu_hdr_line= line("^avg-cpu:  %user   %nice %system %iowait  %steal   %idle$", [])
    avg_cpu_line    = line("^" + "\s+([\d\.]+)" * 6 + "$", [('user', 'f'), ('nice', 'f'), ('system', 'f'), ('iowait', 'f'),  ('steal', 'f'),   ('idle', 'f')])
    dev_hdr_line    = line("^Device:            tps   Blk_read/s   Blk_wrtn/s   Blk_read   Blk_wrtn$", [])
    dev_line        = line("^(\w+)\s+([\d\.]+)\s+([\d\.]+)\s+([\d\.]+)\s+(\d+)\s+(\d+)$", [('device', 's'), ('tps', 'f'), (' Blk_read', 'f'), (' Blk_wrtn', 'f'), (' Blk_read', 'd'), (' Blk_wrtn', 'd')])

    def parse_file(self, file_name):
        res = default_empty_dict()
        data = open(file_name).readlines()
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

            m = read_while([self.dev_line], data)
            for device in m:
                dev_name = device.pop('device')
                for val in device.iteritems():
                    res['dev:' + dev_name + '_' + val[0]] += [val[1]]

        return res

class VMStat(TimeSeries):
    file_hdr_line   = line("^procs -----------memory---------- ---swap-- -----io---- -system-- ----cpu----$", [])
    stats_hdr_line  = line("^ r  b   swpd   free   buff  cache   si   so    bi    bo   in   cs us sy id wa$", [])
    stats_line      = line("\s+(\d+)" * 16, [('r', 'd'),  ('b', 'd'),   ('swpd', 'd'),   ('free', 'd'),   ('buff', 'd'),  ('cache', 'd'),   ('si', 'd'),   ('so', 'd'),    ('bi', 'd'),    ('bo', 'd'),   ('in', 'd'),   ('cs', 'd'), ('us', 'd'), ('sy', 'd'), ('id', 'd'), ('wa', 'd')])

    def parse_file(self, file_name):
        res = default_empty_dict()
        data = open(file_name).readlines()
        data.reverse()
        while True:
            m = until(self.file_hdr_line, data)
            if m == False:
                break
            m = take(self.stats_hdr_line, data)
            assert m != False
            m = read_while([self.stats_line], data)
            for stat_line in m:
                for val in stat_line.iteritems():
                    res[val[0]]+= [val[1]]
        return res

class Latency(TimeSeries):
    line = line("(\d+)\s+([\d.]+)\n", [('tick', 'd'), ('latency', 'f')])

    def parse_file(self, file_name):
        res = default_empty_dict()
        f = open(file_name)
        for line in f:
            res['latency'] += [self.line.parse_line(line)['latency']]
        return res

class QPS(TimeSeries):
    line = line("(\d+)\s+([\d]+)\n", [('tick', 'd'), ('qps', 'f')])

    def parse_file(self, file_name):
        res = default_empty_dict()
        f = open(file_name)
        for line in f:
            res['qps'] += [self.line.parse_line(line)['qps']]
        return res

class RDBStats(TimeSeries):

    stat_line = line("STAT (\S+) (.+)", [('name', 's'), ('value', 's')])
    end_line  = line("END", [])
    
    parsers = [
        line("(\d+)", [('value', 'd')]),
        line("(\d+) \(average of \d+\)", [('value', 'd')]),
        line("\d+/%d+ \((\d+\.\d+)\)", [('value', 'f')]),
        ]
    
    def parse_file(self, file_name):
        res = default_empty_dict()
        data = open(file_name).readlines()
        data.reverse()
        
        while data:
            
            while True:
                m = take_maybe(self.stat_line, data)
                if m == False: break
                
                for parser in self.parsers:
                    m2 = parser.parse_line(m["value"])
                    if m2 != False:
                        res[m["name"]] += [m2["value"]]
                        break
                else:
                    # TODO: Can we do better in the case of parsing failures?
                    res[m["name"]] += [-1]
            
            m = take(self.end_line, data)
            assert m != False
        
        # Make sure that the same stats appear every time, so that
        # the time series are not out of sync
        if res:
            first = res.keys()[0]
            for key in res.keys()[1:]:
                assert len(res[first]) == len(res[key])
        
        return res

    def process(self):
        differences = [('io_reads_complete', 'io_reads_started'), 
                       ('io_writes_started', 'io_writes_completed'), 
                       ('transactions_started', 'transactions_ready'), 
                       ('transactions_ready', 'transactions_completed'),
                       ('bufs_acquired', 'bufs_ready'),
                       ('bufs_ready', 'bufs_released')]
        self.derive('io_reads_complete - io_reads_started', ('io_reads_complete', 'io_reads_started'), difference)

        keys_to_drop = set()
        for dif in differences:
            self.derive(dif[0] + ' - ' + dif[1], dif, difference)
            keys_to_drop.add(dif[0])
            keys_to_drop.add(dif[1])
        self.drop(keys_to_drop)
