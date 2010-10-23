import numpy as np
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
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
        self.data = None

    def read(self, file_name):
        self.data = self.parse_file(file_name)

    def copy(self):
        pass

    def __add__(self, other):
        res = self.copy()
        for val in other.data.iteritems():
            assert not val[0] in res.data
            res.data[val[0]] = val[1]
        return res
        
    def parse_file(self, file_name):
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
            labels.append((ax.plot(range(len(series[1])), normalize(series[1]), colors[color_index]), series[0]))
            color_index += 1

        plt.figlegend(tuple(map(lambda x: x[0], labels)), tuple(map(lambda x: x[1], labels)), 'upper right', shadow=True)
        ax.set_xlabel('Time (seconds)')
        ax.set_xlim(0, len(self.data[self.data.keys()[0]]) - 1)
        ax.set_ylim(0, 1.0)
        ax.grid(True)
        plt.savefig(out_fname, dpi=300)

class IOStat(TimeSeries):
    file_hdr_line   = line("Linux.*", [])
    avg_cpu_hdr_line= line("^avg-cpu:  %user   %nice %system %iowait  %steal   %idle$", [])
    avg_cpu_line    = line("^" + "\s+([\d\.]+)" * 6 + "$", [('user', 'f'), ('nice', 'f'), ('system', 'f'), ('iowait', 'f'),  ('steal', 'f'),   ('idle', 'f')])
    dev_hdr_line    = line("^Device:            tps   Blk_read/s   Blk_wrtn/s   Blk_read   Blk_wrtn$", [])
    dev_line        = line("^(\w+)\s+([\d\.]+)\s+([\d\.]+)\s+([\d\.]+)\s+(\d+)\s+(\d+)$", [('device', 's'), ('tps', 'f'), (' Blk_read', 'f'), (' Blk_wrtn', 'f'), (' Blk_read', 'd'), (' Blk_wrtn', 'd')])

    def copy(self):
        copy = IOStat()
        copy.data = self.data.copy()
        return copy

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

    def copy(self):
        copy = VMStat()
        copy.data = self.data.copy()
        return copy

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

    def copy(self):
        copy = Latency()
        copy.data = self.data.copy()
        return copy

    def parse_file(self, file_name):
        res = default_empty_dict()
        f = open(file_name)
        for line in f:
            res['latency'] += [self.line.parse_line(line)['latency']]
        return res

#    def histogram(self, out_fname):
#        fig = plt.figure()
#        ax = fig.add_subplot(111)
#        ax.hist(self.data, 400, range=(0, (sum(self.data) / len(self.data)) * 2), facecolor='green', alpha=1.0)
#        ax.set_xlabel('Latency')
#        ax.set_ylabel('Count')
#        ax.set_xlim(0, (sum(self.data) / len(self.data)) * 2)
#        ax.set_ylim(0, len(self.data) / 60)
#        ax.grid(True)
#        plt.savefig(out_fname)
#    def scatter(self, out_fname):
#        fig = plt.figure()
#        ax = fig.add_subplot(111)
#        ax.plot(range(len(self.data)), self.data, 'g.-')
#        ax.set_xlabel('Time (seconds)')
#        ax.set_ylabel('latency')
#        ax.set_xlim(0, len(self.data))
#        ax.set_ylim(0, max(self.data) * 1.2)
#        ax.grid(True)
#        plt.savefig(out_fname)
#    def series_hist(self, out_fname):
#        fig = plt.figure(1, figsize=(10.0, 10.0))
#        ax = fig.add_subplot(111)
#        ax.plot(range(len(self.data)), self.data, 'g.-')

class QPS(TimeSeries):
    line = line("(\d+)\s+([\d]+)\n", [('tick', 'd'), ('qps', 'f')])

    def copy(self):
        copy = QPS()
        copy.data = self.data.copy()
        return copy

    def parse_file(self, file_name):
        res = default_empty_dict()
        f = open(file_name)
        for line in f:
            res['qps'] += [self.line.parse_line(line)['qps']]
        return res

#    def histogram(self, out_fname):
#        fig = plt.figure()
#        ax = fig.add_subplot(111)
#        ax.hist(self.data, 400, range=(0, (sum(self.data) / len(self.data)) * 2), facecolor='green', alpha=1.0)
#        ax.set_xlabel('QPS')
#        ax.set_ylabel('Count')
#        ax.set_xlim(0, (sum(self.data) / len(self.data)) * 2)
#        ax.set_ylim(0, len(self.data) / 60)
#        ax.grid(True)
#        plt.savefig(out_fname)
#    def scatter(self, out_fname):
#        fig = plt.figure()
#        ax = fig.add_subplot(111)
#        ax.plot(range(len(self.data)), self.data, 'g.-')
#        ax.set_xlabel('Time (seconds)')
#        ax.set_ylabel('QPS')
#        ax.set_xlim(0, len(self.data))
#        ax.set_ylim(0, max(self.data) * 1.2)
#        ax.grid(True)
#        plt.savefig(out_fname)

class RDBStats(TimeSeries):
    cmd_set_line        = line("STAT cmd_set (\d+)", [('sets', 'd')])
    evts_p_loop_line    = line("STAT events_per_loop (\d+) \(average of \d+\)", [('events_per_loop', 'd')])
    end_line            = line("END", [])

    def copy(self):
        copy = RDBStats()
        copy.data = self.data.copy()
        return copy

    def parse_file(self, file_name):
        res = default_empty_dict()
        data = open(file_name).readlines()
        data.reverse()
        while True:
            m = take(self.cmd_set_line, data)
            if m == False:
                break
            
            for val in m.iteritems():
                res[val[0]] += [val[1]]

            m = take(self.evts_p_loop_line, data)
            assert m

            for val in m.iteritems():
                res[val[0]] += [val[1]]

            m = take(self.end_line, data)
            assert m != False

        return res
