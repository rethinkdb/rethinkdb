#!/usr/bin/env python
from __future__ import with_statement
import subprocess, sys, os, tempfile, shlex
from time import sleep
from optparse import OptionParser
from copy import deepcopy
from collections import defaultdict

parser = OptionParser()
parser.add_option("-d","--duration",dest="duration",action="store")
parser.add_option("-c","--threads",dest="threads",action="store", type="int")
parser.add_option("-b","--block_size",dest="block_size",action="store", type="int")
parser.add_option("-s","--stride",dest="stride",action="store", type="int")
parser.add_option("-w","--workload",dest="workload",action="store")
parser.add_option("-t","--type",dest="type",action="store")
parser.add_option("-q","--queue-depth",dest="queue-depth",action="store", type="int")
parser.add_option("--eventfd",dest="eventfd",action="store")
parser.add_option("-r","--direction",dest="direction",action="store")
parser.add_option("-o","--operation",dest="operation",action="store")
parser.add_option("-p","--paged",dest="paged",action="store")
parser.add_option("-f","--buffered",dest="buffered",action="store")
parser.add_option("-m","--do-atime",dest="do-atime",action="store")
parser.add_option("-a","--append",dest="append",action="store")
parser.add_option("-u","--dist",dest="dist",action="store")
parser.add_option("-i","--sigma",dest="sigma",action="store", type="int")
parser.add_option("-l","--local-fd",dest="local-fd",action="store")
parser.add_option("-j","--offset",dest="offset",action="store", type="int")
parser.add_option("-e","--length",dest="length",action="store")
parser.add_option("-n","--silent",dest="silent",action="store")
parser.add_option("-g","--sample-step",dest="sample-step",action="store", type="int")
parser.add_option("--drop-caches",dest="drop-caches",action="store")
parser.add_option("--output",dest="output",action="store")


class Result(object):
    """
        This class contains information on one run of Rebench.
        
        self.command            - the command used to invoke Rebench
        self.flags              - a dictionary containing the value for each flag that can be passed to rebench
        self.samples            - a list of samples gotten from Rebench for this run

        This class overloads __repr__, which prints out the result in the format:
        [command used to run Rebench]
        [results]
        [" " separated samples]
        
    """
    def __init__(self, _command, _ops_per_sec, _mb_per_sec, _min_ops_per_sec, _max_ops_per_sec, _std_dev, _samples=None):
        self.ops_per_sec = _ops_per_sec
        self.mb_per_sec = _mb_per_sec
        self.min_ops_per_sec = _min_ops_per_sec
        self.max_ops_per_sec = _max_ops_per_sec
        self.std_dev = _std_dev
        self.command = _command
        self.samples = _samples
        self.parse_args()

    def parse_args(self):
        assert(self.command)
        (flags, args) = parser.parse_args(self.command.split(" "))
        self.flags = flags # now self.flags is a dictionary that contains all the f((lags we specified and the values we specified

    def __repr__(self):
        return "%s\n%s %s %s %s %s\n%s\n" %  (self.command, self.ops_per_sec, self.mb_per_sec, self.min_ops_per_sec, self.max_ops_per_sec, self.std_dev, " ".join(self.samples))

    def add_samples(self, samples):
        self.samples = deepcopy(samples)


def analyze_data(filename):
    """
        Given a file of data collected via run_rebench(),
        deserializes it to Result objects, making the data
        easy to use.

        Returns the list of Result objects.
    """
    print "Analyzing data."
    datafile = open(filename,"r").readlines()
    results = []
    cmd = ""
    
    # data is stored in the format [command] \n [results] \n [samples]
    for i in range(len(datafile)):
        # command
        if i % 3 == 0:
            cmd = datafile[i].rstrip()
        elif i % 3 == 1:
            # results
            tokens = datafile[i].split(' ')
            tokens[-1] = tokens[-1].rstrip()
            results.append(Result(cmd,*tokens))
        else:
            # samples
            detailed = datafile[i].split(" ")
            detailed[-1] = detailed[-1].rstrip()
            results[-1].add_samples(detailed)

    return results


def sort_data_by_ops_per_sec(results):
    """
        Given a list of Result objects, sorts them
        by operations per second, descending.
    """
    print "Sorting data."
    return sorted(results, key=lambda res: res.ops_per_sec, reverse=True)


def run_rebench(filename, **flags):
    """
        Runs Rebench for the all the specified parameters.
    """
    print "Running rebench."

    datafile    = open(filename,"w+")
    (_, temp_file_name) =  tempfile.mkstemp()
    
    flags.setdefault("output",temp_file_name)
    
    flags_string = ""
    for key in flags:
        flags_string += " --%s %s" % (key, str(flags[key]))

    cmd = "rebench %s --silent /dev/sdb" % flags_string
    print "Running %s" % cmd

    # write and force write to disk
    datafile.write(cmd+"\n")
    datafile.flush()
    os.fsync(datafile.fileno())
    
    proc = subprocess.Popen(shlex.split(cmd, " "), stdout=datafile, close_fds=True)
    proc.wait()                    
    
    # we really want that detailed output to be in our datafile
    detailed = open(flags["output"],"r").readlines()
    detailed = [d.rstrip() for d in detailed]
    datafile.write(" ".join(detailed) + "\n")
    print "Done."

    # delete temporary file
    os.remove(temp_file_name)
    print "Finished running rebench."
    datafile.close()


def generate_graph_data(filename, data_file):
    """
        Given a file containing data generated by run_rebench(),
        creates a data file of sample values over the # of samples.
        You can use gnuplot to create a graph of this data file.
    """
    print "creating data file"
    results = analyze_data(filename)
    assert(len(results)==1) # we can only make the graph for one test.
    with open(data_file,"w+") as f:
        f.write("#" + results[0].command)
        for i in range(len(results[0].samples)):
            f.write(str(i) + " " + results[0].samples[i] + "\n")
    print "done."


def generate_histogram_data(filename, data_file):
    """
        Given a file containing data generated by run_rebench(),
        creates a file containing the data for a histogram of the
        samples of the given result.
    """    
    print "creating histogram data file"
    
    results = analyze_data(filename)
    assert(len(results)==1) # we can only make the histogram for one test.
    count = defaultdict(lambda: 0)
    
    bucket_size = 50
    for r in results:
        for i in range(len(r.samples)):
            key = (int(r.samples[i]) // bucket_size) * bucket_size # bin size, round to nearest bucket_size
            count[key] += 1

    with open(data_file,"w+") as f:
        f.write("#" + results[0].command)
        for k in sorted(count):
            f.write(str(k) + " " + str(count[k]) + "\n")
    print "done."


def generate_graph(data_file):
    print "creating plot"
    title = open(data_file,"r").readline().rstrip().replace("#","")
     # make the title fit on the page
    for i in range(0,len(title),50):
        title = title[:i] + "\n" + title[i:]
    cmd = "gnuplot -e \"set title '%s'; set xlabel 'sample number'; set ylabel 'Ops/sec'; set terminal gif; set output '%s.gif'; plot '%s'\"" % (title, os.path.splitext(data_file)[0], data_file)
    p = subprocess.Popen(shlex.split(cmd), stdout=sys.stdout, close_fds=True)
    p.wait()
    print "done."


def generate_histogram(data_file):
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
    
    title = data[0].rstrip().replace("#","")
    # make the title fit on the page
    for i in range(0,len(title),50):
        title = title[:i] + "\n" + title[i:]
    cmd = "gnuplot -e \"set title '%s'; set terminal gif; set ylabel 'Frequency'; set xlabel 'Ops/sec'; set xrange [%d:%d]; set yrange [%d:%d]; set output '%s.gif'; plot '%s' with boxes\"" % (title, min(x) - 10, max(x) + 10, min(y) - 10, max(y) + 10, os.path.splitext(data_file)[0], data_file)
    p = subprocess.Popen(shlex.split(cmd), stdout=sys.stdout, close_fds=True)
    p.wait()
    print "done."


def main(results_file, graph_data_file, flags):
    run_rebench(results_file, **flags)
    results = analyze_data(results_file)
    
    (base, ext) = os.path.splitext(graph_data_file)
    histogram_file = base + "_histogram" + ext
 
    generate_graph_data(results_file, graph_data_file)
    generate_histogram_data(results_file, histogram_file)
    generate_graph(graph_data_file)
    generate_histogram(histogram_file)


if __name__ == "__main__":
    if len(sys.argv) < 3:
        raise SystemExit, "usage: %s [output file (test data)] [output file (graph data)]" % sys.argv[0]

    (flags, _) = parser.parse_args()
    flags = vars(flags)
    for k in flags.keys():
        if flags[k] is None: del flags[k]

    flags.setdefault("threads",1)
    flags.setdefault("block_size",512)
    flags.setdefault("stride",512)
    flags.setdefault("workload","rnd")
    flags.setdefault("operation","write")
    flags.setdefault("duration","2")
    flags.setdefault("type","stateless")
    flags.setdefault("sample-step",50)

    main(sys.argv[1], sys.argv[2], flags)

