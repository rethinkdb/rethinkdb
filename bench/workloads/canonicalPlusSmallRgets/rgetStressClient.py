#!/usr/bin/env python
# Copyright 2010-2012 RethinkDB, all rights reserved.

import optparse, time, sys, random

documentation = """
XXX TODO
This stress client runs the canonical workload. In addition, it runs a number of comparably small rgets on a separate set of clients.
QPS stats are collected independently for rgets and other operations."""

parser = optparse.OptionParser(description = documentation)
parser.add_option("--client-suffix", dest="client_suffix", action="store_true", default=False,
    help="For compatibility with the stress client. Ignored.")
parser.add_option("-q", "--qps", dest="qps", default=None,
    help="Filename to put QPS output in. Each line will have a time and a QPS count.",
    metavar="FILENAME")
parser.add_option("-l", "--latencies", dest="latencies", default=None,
    help="Filename to put latency samples in. Each line will have a time and a latency in seconds.",
    metavar="FILENAME")
parser.add_option("-s", "--server", dest="servers", action="append",
    help="Server to run against, in stress-client command-string format. If multiple are "
    "specified, queries will be split up among them.", metavar="CONN_STR")
parser.add_option("-w", "--workload", dest="workload",
    help="WORKLOAD must be the string 'special_rget_scan_workload'.", metavar="WORKLOAD")

(options, args) = parser.parse_args()

# Validate options

assert options.servers is not None
assert options.workload == "special_rget_workload"

# The workload parameters are hard-coded.

duration = (1800, "seconds")
assert options.servers[0]
if options.servers[0].startswith("mysql,"):
    num_clients = 96
    num_rget_clients = 84
else:
    num_clients = 512
    num_rget_clients = 448
num_canonical_clients = num_clients - num_rget_clients
keys = (8,16)
values = (8,128)
delete_freq = 1
update_freq = 4
insert_freq = 8
read_freq = 64 / 8 # We have to divide by the average batch factor

rget_size = (16,64)

# Before running this script, full_bench copies it into a directory with libstress.so
# and stress.py.

import stress

# Set up any MySQL tables if necessary

assert options.servers
for server in options.servers:
    if server.startswith("mysql,"):
        protocol, server = server.split(",")
        stress.initialize_mysql_table(server, keys[1], values[1])

# Open output files

qps_file_rget = None
qps_file_canonical = None
if options.qps is None: qps_file = None
elif options.qps == "-": qps_file = sys.stdout
else: 
    qps_file = file(options.qps, "w")
    # This is quite hacky...
    qps_file_rget = file(options.qps + ".rget", "w")
    qps_file_canonical = file(options.qps + ".canonical", "w")

if options.latencies is None: latencies_file = None
elif options.latencies == "-": latencies_file = sys.stdout
else: latencies_file = file(options.latencies, "w")

# Set up the clients

class CanonicalClient(object):
    def __init__(self, i):
        self.key_generator = stress.SeedKeyGenerator((i, num_clients), keysize=keys)
        self.model = stress.ConsecutiveSeedModel()
        self.connection = stress.Connection(options.servers[i % len(options.servers)])
        self.client = stress.Client()
        self.insert_op = stress.InsertOp(self.key_generator, self.model.insert_chooser(), self.model, self.connection, values)
        self.client.add_op(insert_freq, self.insert_op)
        self.update_op = stress.UpdateOp(self.key_generator, self.model.live_chooser(), self.model, self.connection, values)
        self.client.add_op(update_freq, self.update_op)
        self.delete_op = stress.DeleteOp(self.key_generator, self.model.delete_chooser(), self.model, self.connection)
        self.client.add_op(delete_freq, self.delete_op)
        self.read_op = stress.ReadOp(self.key_generator, self.model.live_chooser(), self.connection, (1, 16) )
        self.client.add_op(read_freq, self.read_op)
    def poll_and_reset(self):
        queries = 0
        latencies = []
        for op in [self.insert_op, self.update_op, self.delete_op, self.read_op]:
            op.lock()
            stats = op.poll()
            op.reset()
            op.unlock()
            queries += stats["queries"]
            latencies.extend(stats["latency_samples"])
            if op is self.insert_op: inserts = stats["queries"]
        return {"queries": queries, "latency_samples": latencies, "inserts": inserts}
        
class RgetClient(object):
    def __init__(self, i):
        self.key_generator = stress.SeedKeyGenerator((i, num_clients), keysize=keys)
        self.model = stress.ConsecutiveSeedModel()
        self.connection = stress.Connection(options.servers[i % len(options.servers)])
        self.client = stress.Client()
        self.rget_op = stress.PercentageRangeReadOp(self.connection, percentage=(1,2), limit=rget_size )
        self.client.add_op(1, self.rget_op)
    def poll_and_reset(self):
        queries = 0
        latencies = []
        for op in [self.rget_op]:
            op.lock()
            stats = op.poll()
            op.reset()
            op.unlock()
            queries += stats["queries"]
            latencies.extend(stats["latency_samples"])
        return {"queries": queries, "latency_samples": latencies, "inserts": 0}

clients = [CanonicalClient(i) for i in xrange(num_canonical_clients)]
rget_clients = [RgetClient(i) for i in xrange(num_rget_clients)]

# Start the clients

print "Running special canonical + small rget workload."
for c in clients: c.client.start()
for c in rget_clients: c.client.start()

# Loop

start_time = time.time()
total_queries = 0
total_inserts = 0
total_time = 0

while True:

    # Delay an integer number of seconds, preferably 1
    round_time = 1
    while start_time + total_time + round_time < time.time(): round_time += 1
    time.sleep(start_time + total_time + round_time - time.time())

    # Get stats from each client
    round_queries = 0
    # (also split up qps based on operations)
    round_queries_canonical = 0
    round_queries_rget = 0
    round_inserts = 0
    round_latencies = []
    for c in clients:
        stats = c.poll_and_reset()
        round_queries += stats["queries"]
        round_queries_canonical += stats["queries"]
        round_inserts += stats["inserts"]
        round_latencies.extend(stats["latency_samples"])
    for c in rget_clients:
        stats = c.poll_and_reset()
        round_queries += stats["queries"]
        round_queries_rget += stats["queries"]
        # TODO: Record rget latency

    # Write stats out
    if qps_file:
        qps_file.write("%d\t%d\n" % (total_time, round_queries))
    if qps_file_rget:
        qps_file_rget.write("%d\t%d\n" % (total_time, round_queries_rget))
    if qps_file_canonical:
        qps_file_canonical.write("%d\t%d\n" % (total_time, round_queries_canonical))
    if latencies_file:
        # Choose 20 (or less) latency samples to write
        samples = random.sample(round_latencies, min(20, len(round_latencies)))
        for sample in samples:
            latencies_file.write("%d\t%d\n" % (total_time, sample * 1000000))

    # Update running totals
    total_queries += round_queries
    total_inserts += round_inserts
    total_time += round_time

    # Determine if we should stop
    if {"queries": total_queries, "inserts": total_inserts, "seconds": total_time}[duration[1]] > duration[0]:
        break

