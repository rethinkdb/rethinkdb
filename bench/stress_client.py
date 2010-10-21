import numpy as np
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
import re

class line():
    regex = ""
    fields = [] #tuples of form name, type
    def __init__(self, _regex, _fields):
        self.regex = _regex
        self.fields = _fields
    def parse_line(self, line):
        matches = re.match(self.regex, line)
        if matches:
            result = {}
            for field, groupi in zip(self.fields, range(1, len(self.fields) + 1)):
                if (field[1] == 'd'):
                    val = int(matches.group(groupi))
                elif (field[1] == 'f'):
                    val = float(matches.group(groupi))
                else:
                    assert 0
                result[field[0]] = val
            return result
        else:
            return False

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
        ax.scatter(range(len(self.data)), self.data, marker='o', c='g')
        ax.set_xlabel('Time (seconds)')
        ax.set_ylabel('latency')
        ax.set_xlim(0, len(self.data))
        ax.set_ylim(0, max(self.data) * 1.2)
        ax.grid(True)
        plt.savefig(out_fname)


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
        ax.scatter(range(len(self.data)), self.data, marker='o', c='g')
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
