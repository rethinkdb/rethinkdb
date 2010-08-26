#!/usr/bin/env python
from __future__ import with_statement
from bench_test import *
import subprocess, sys, shlex, os
from collections import defaultdict

def create_data_file(filename, data_file):
    print "creating data file"
    results = analyze_data(filename)
    assert(len(results)==1) # we can only make the graph for one test.
    with open(data_file,"w+") as f:
        f.write("#" + results[0].command)
        for i in range(len(results[0].flags)):
            f.write(str(i) + " " + results[0].flags[i] + "\n")
    print "done."


def create_hist_data_file(filename, data_file):
    print "creating histogram data file"
    
    results = analyze_data(filename)
    assert(len(results)==1) # we can only make the histogram for one test.
    count = defaultdict(lambda: 0)
    
    for r in results:
        for i in range(len(r.flags)):
            key = (int(r.flags[i])//10) * 10 # bin size, round to nearest tenth
            count[key] += 1

    with open(data_file,"w+") as f:
        f.write("#" + results[0].command)
        for k in sorted(count):
            f.write(str(k) + " " + str(count[k]) + "\n")
    print "done."


def create_plot(data_file):
    print "creating plot"
    title = open(data_file,"r").readline().rstrip()
    cmd = "gnuplot -e \"set title '%s'; set terminal gif; set output '%s.gif'; plot '%s'\"" % (title, os.path.splitext(data_file)[0], data_file)
    p = subprocess.Popen(shlex.split(cmd), stdout=sys.stdout, close_fds=True)
    p.wait()
    print "done."


def create_hist_plot(data_file):
    print "creating histogram plot"
 
    # first we need to figure out the range of the data
    data = open(data_file,"r").readlines()
    x = []
    y = []
    
    for line in data:
        if line[0] == "#": continue # skip comments
        tokens = line.split()
        x.append(int(tokens[0]))
        y.append(int(tokens[1].rstrip()))
    
    title = data[0].rstrip()
    cmd = "gnuplot -e \"set title '%s'; set terminal gif; set ylabel 'Frequency'; set xlabel 'Ops/sec'; set xrange [%d:%d]; set yrange [%d:%d]; set output '%s.gif'; plot '%s' with boxes\"" % (title, min(x) - 10, max(x) + 10, min(y) - 10, max(y) + 10, os.path.splitext(data_file)[0], data_file)
    p = subprocess.Popen(shlex.split(cmd), stdout=sys.stdout, close_fds=True)
    p.wait()
    print "done."


if __name__ == "__main__":
    if len(sys.argv) < 3:
        raise SystemExit, "usage: %s [input file] [output file]" % sys.argv[0]
    input = sys.argv[1]
    output = sys.argv[2]
    (base, ext) = os.path.splitext(output)
    histogram_file = base + "_histogram" + ext
 
    create_data_file(input, output)
    create_hist_data_file(input, histogram_file)
    create_plot(output)
    create_hist_plot(histogram_file)
