import numpy as np
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
import re
from mpl_toolkits.axes_grid import make_axes_locatable

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
        copy = default_zero_dict()
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
    def __init__(self, file_name):
        pass
        
    def parse_file(self, file_name):
        pass

    def histogram(self, out_fname):
        pass

    def plot(self, out_fname):
        pass

class IOStat(TimeSeries):
    file_hdr_line   = line("Linux.*", [])
    avg_cpu_hdr_line= line("^avg-cpu:  %user   %nice %system %iowait  %steal   %idle$", [])
    avg_cpu_line    = line("^" + "\s+([\d\.]+" * 6 + "$", [('user', 'f'), ('nice', 'f'), ('system', 'f'), ('iowait', 'f'),  ('steal', 'f'),   ('idle', 'f')])
    dev_hdr_line    = line("^Device:            tps   Blk_read/s   Blk_wrtn/s   Blk_read   Blk_wrtn$", [])
    dev_line        = line("^(\w+)\s+([\d\.]+)\s+([\d\.]+)\s+([\d\.]+)\s+(\d+)\s+(\d+)$", [('device', 's'), ('tps', 'f'), (' Blk_read', 'f'), (' Blk_wrtn', 'f'), (' Blk_read', 'd'), (' Blk_wrtn', 'd')])

    def __init__(self, file_name):
        self.data = self.parse_file(file_name)

    def parse_file(self, file_name):
        res = default_empty_dict()
        data = open(file_name).readlines()
        data.reverse()
        m = until(self.file_hdr_line, data)
        assert m != False
        while True:
            m = until(self.avg_cpu_hdr_line, data)
            if not m:
                break

            m = take(self.avg_cpu_line, data)
            assert m
            for val in m.iteritems():
                res['cpu_' + val[0]].append(val[1])

            m = read_while([self.dev_hdr_line], data)
            assert m
            for device in m:
                dev_name = device.pop('device')
                for val in device.iteritems():
                    res['dev:' + dev_name + '_' + val[0]].append(val[1])

        return res

    def histogram(self, out_fname):
        pass

    def plot(self, out_fname):
        fig = plt.figure()
        ax = fig.add_subplot(111)
        for series in self.data.iteritems():
            ax.plot(range(len(series)), normalize(series), 'g.-')
        ax.set_xlabel('Time (seconds)')
        ax.set_xlim(0, len(self.data))
        ax.set_ylim(0, 1.0)
        ax.grid(True)
        plt.savefig(out_fname)


class VMStat():
    file_hdr_line   = line("^procs -----------memory---------- ---swap-- -----io---- -system-- ----cpu----$", [])
    stats_hdr_line  = line("^r  b   swpd   free   buff  cache   si   so    bi    bo   in   cs us sy id wa$", [])
    stats_line      = line("^" + "\s+(\d+)" * 14 + "$", [('r', 'd'),  ('b', 'd'),   ('swpd', 'd'),   ('free', 'd'),   ('buff', 'd'),  ('cache', 'd'),   ('si', 'd'),   ('so', 'd'),    ('bi', 'd'),    ('bo', 'd'),   ('in', 'd'),   ('cs', 'd'), ('us', 'd'), ('sy', 'd'), ('id', 'd'), ('wa', 'd')])

    def __init__(self, file_name):
        self.data = self.parse_file(file_name)

    def parse_file(self, file_name):
        res = default_empty_dict()
        data = open(file_name).readlines()
        data.reverse()
        m = until(self.file_hdr_line, data)
        assert m
        m = take(self.stats_hdr_line, data)
        assert m
        m = read_while([self.stats_line], data)
        for stat_line in m:
            for val in stat_line.iteritems():
                res[val[0]].append(val[1])
        return res

class Latency():
    line = line("(\d+)\s+([\d.]+)\n", [('tick', 'd'), ('latency', 'f')])
    def __init__(self, file_name):
        self.data = self.parse_file(file_name)
    def parse_file(self, file_name):
        res = []
        f = open(file_name)
        for line in f:
            res.append(self.line.parse_line(line)['latency'])
        return res
    def histogram(self, out_fname):
        fig = plt.figure()
        ax = fig.add_subplot(111)
        ax.hist(self.data, 400, range=(0, (sum(self.data) / len(self.data)) * 2), facecolor='green', alpha=1.0)
        ax.set_xlabel('Latency')
        ax.set_ylabel('Count')
        ax.set_xlim(0, (sum(self.data) / len(self.data)) * 2)
        ax.set_ylim(0, len(self.data) / 60)
        ax.grid(True)
        plt.savefig(out_fname)
    def scatter(self, out_fname):
        fig = plt.figure()
        ax = fig.add_subplot(111)
        ax.plot(range(len(self.data)), self.data, 'g.-')
        ax.set_xlabel('Time (seconds)')
        ax.set_ylabel('latency')
        ax.set_xlim(0, len(self.data))
        ax.set_ylim(0, max(self.data) * 1.2)
        ax.grid(True)
        plt.savefig(out_fname)
    def series_hist(self, out_fname):
        fig = plt.figure(1, figsize=(10.0, 10.0))
        ax = fig.add_subplot(111)
        ax.plot(range(len(self.data)), self.data, 'g.-')



def process_latency(fin, fout):
    l = Latency(fin)
    l.histogram(fout + '_histogram')
    l.scatter(fout + '_scatter')

class QPS():
    line = line("(\d+)\s+([\d]+)\n", [('tick', 'd'), ('qps', 'f')])
    def __init__(self, file_name):
        self.data = self.parse_file(file_name)
    def parse_file(self, file_name):
        res = []
        f = open(file_name)
        for line in f:
            res.append(self.line.parse_line(line)['qps'])
        return res
    def histogram(self, out_fname):
        fig = plt.figure()
        ax = fig.add_subplot(111)
        ax.hist(self.data, 400, range=(0, (sum(self.data) / len(self.data)) * 2), facecolor='green', alpha=1.0)
        ax.set_xlabel('QPS')
        ax.set_ylabel('Count')
        ax.set_xlim(0, (sum(self.data) / len(self.data)) * 2)
        ax.set_ylim(0, len(self.data) / 60)
        ax.grid(True)
        plt.savefig(out_fname)
    def scatter(self, out_fname):
        fig = plt.figure()
        ax = fig.add_subplot(111)
        ax.plot(range(len(self.data)), self.data, 'g.-')
        ax.set_xlabel('Time (seconds)')
        ax.set_ylabel('QPS')
        ax.set_xlim(0, len(self.data))
        ax.set_ylim(0, max(self.data) * 1.2)
        ax.grid(True)
        plt.savefig(out_fname)

def process_qps(fin, fout):
    q = QPS(fin)
    q.histogram(fout + '_histogram')
    q.scatter(fout + '_scatter')
