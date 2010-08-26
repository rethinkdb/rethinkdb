#!/usr/bin/env python
from __future__ import with_statement
import subprocess, sys, os, tempfile
from time import sleep
from optparse import OptionParser
from copy import deepcopy


class Result(object):
    def __init__(self, _command, _ops_per_sec, _mb_per_sec, _min_ops_per_sec, _max_ops_per_sec, _std_dev, _flags=None):
        self.ops_per_sec = _ops_per_sec
        self.mb_per_sec = _mb_per_sec
        self.min_ops_per_sec = _min_ops_per_sec
        self.max_ops_per_sec = _max_ops_per_sec
        self.std_dev = _std_dev
        self.command = _command
        self.flags = _flags
        self.parse_args()

    def parse_args(self):
        assert(self.command)
        parser = OptionParser()
        parser.add_option("-d","--duration",dest="duration",action="store")
        parser.add_option("-c","--threads",dest="threads",action="store")
        parser.add_option("-b","--block_size",dest="block_size",action="store")
        parser.add_option("-s","--stride",dest="stride",action="store")
        parser.add_option("-w","--workload",dest="workload",action="store")
        parser.add_option("-t","--type",dest="type",action="store")
        parser.add_option("-q","--queue-depth",dest="queue-depth",action="store")
        parser.add_option("--eventfd",dest="eventfd",action="store")
        parser.add_option("-r","--direction",dest="direction",action="store")
        parser.add_option("-o","--operation",dest="operation",action="store")
        parser.add_option("-p","--paged",dest="paged",action="store")
        parser.add_option("-f","--buffered",dest="buffered",action="store")
        parser.add_option("-m","--do-atime",dest="do-atime",action="store")
        parser.add_option("-a","--append",dest="append",action="store")
        parser.add_option("-u","--dist",dest="dist",action="store")
        parser.add_option("-i","--sigma",dest="sigma",action="store")
        parser.add_option("-l","--local-fd",dest="local-fd",action="store")
        parser.add_option("-n","--silent",dest="silent",action="store")
        parser.add_option("-j","--offset",dest="offset",action="store")
        parser.add_option("-e","--length",dest="length",action="store")
        parser.add_option("-g","--sample-step",dest="sample-step",action="store")
        parser.add_option("--drop-caches",dest="drop-caches",action="store")
        parser.add_option("--output",dest="output",action="store")
        (options, args) = parser.parse_args(self.command.split(" "))
        self.parsed_cmd = options # now self.parsed_cmd is a dictionary that contains all the f((lags we specified and the values we specified

    def __repr__(self):
        return "%s\n%s %s %s %s %s\n%s\n" %  (self.command, self.ops_per_sec, self.mb_per_sec, self.min_ops_per_sec, self.max_ops_per_sec, self.std_dev, " ".join(self.flags))

    def add_flags(self, flags):
        self.flags = deepcopy(flags)


def analyze_data(filename):
    print "analyzing data."
    datafile = open(filename,"r").readlines()
    results = []
    cmd = ""

    for i in range(len(datafile)):
        if i % 3 == 0:
            cmd = datafile[i].rstrip()
        elif i % 3 == 1:
            tokens = datafile[i].split(' ')
            tokens[-1] = tokens[-1].rstrip()
            results.append(Result(cmd,*tokens))
        else:
            detailed = datafile[i].split(" ")
            detailed[-1] = detailed[-1].rstrip()
            results[-1].add_flags(detailed)

    return results


def sort_data(results):
    print "sorting data."
    return sorted(results, key=lambda res: res.ops_per_sec, reverse=True)


def run_rebench(filename):
    print "running rebench."
    parallelism = [1]#,4,8]
    block_size  = [512]#, 1024, 2048]
    stride      = [512]#, 1024, 2048]
    workload    = ["rnd"]#, "seq"]
    operation   = ["read"]#, "write"]
    datafile = open(filename,"w+")
    (_, temp_file_name) =  tempfile.mkstemp()
    for num_threads in parallelism:
        for b_size in block_size:
            for stride_size in stride:
                for work_type in workload:
                    for op in operation:
                        cmd = "rebench -d600 -g50 -n -t stateless -c %d -b %d -s %d -w %s -o %s --output %s /dev/sdb" % (num_threads, b_size, stride_size, work_type, op, temp_file_name)
                        print "running %s" % cmd
                        # write and force write to disk
                        datafile.write(cmd+"\n")
                        datafile.flush()
                        os.fsync(datafile.fileno())
                        proc = subprocess.Popen(cmd.split(" "), stdout=datafile, close_fds=True)
                        proc.wait()                    
                        # we really want that detailed output to be in our datafile
                        detailed = open(temp_file_name,"r").readlines()
                        detailed = [d.rstrip() for d in detailed]
                        datafile.write(" ".join(detailed) + "\n")
                        # delete temporary file
                        os.remove(temp_file_name)

                        print "done."
                           
    print "Finished running rebench."
    datafile.close()


def main(filename):
    run_rebench(filename)
    results = analyze_data(filename)
    sorted_data = sort_data(results)
    
    with open("sorted_"+filename,"w+") as sorted_file:
        for result in sorted_data:
            sorted_file.write(repr(result))


if __name__ == "__main__":
    if len(sys.argv) < 2:
        raise SystemExit, "usage: %s [filename to store data in]" % sys.argv[0]
    main(sys.argv[1])

# We also want to do this to see how the rate of data transfer fluctuates:
# (while true; do cat rand; done) | pv -r > /dev/sdb
