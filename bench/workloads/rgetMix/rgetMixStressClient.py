#!/usr/bin/env python
# Copyright 2010-2012 RethinkDB, all rights reserved.

import optparse, time, sys, random

documentation = """
The rget-mix workload cannot be expressed using the normal stress client, because it
contains range-gets of two different sizes. This script mimics the stress client enough
to be compatible with dbench. It accepts a subset of the parameters that the stress client
accepts, but its "workload" parameter must always be "-w special_rget_mix_workload" and
the workload itself is hard-coded."""

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
    help="WORKLOAD must be the string 'special_rget_mix_workload'.", metavar="WORKLOAD")

(options, args) = parser.parse_args()

# Validate options

assert options.servers is not None
assert options.workload == "special_rget_mix_workload"

# The workload parameters are hard-coded.

duration = (900, "seconds")
assert options.servers[0]
if options.servers[0].startswith("mysql,"):
    num_clients = 96
else:
    num_clients = 512
keys = (8,16)
values = (8,128)
insert_freq = 30000
# Run a lot of small rgets and a very occasional large one
small_rget_freq = 10000
small_rget_size = (8,128)
large_rget_freq = 1
large_rget_size = (100000,100000)

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

if options.qps is None: qps_file = None
elif options.qps == "-": qps_file = sys.stdout
else: qps_file = file(options.qps, "w")

if options.latencies is None: latencies_file = None
elif options.latencies == "-": latencies_file = sys.stdout
else: latencies_file = file(options.latencies, "w")

# Set up the clients

class RgetMixClient(object):
    def __init__(self, i):
        self.key_generator = stress.SeedKeyGenerator((i, num_clients), keysize=keys)
        self.model = stress.ConsecutiveSeedModel()
        self.connection = stress.Connection(options.servers[i % len(options.servers)])
        self.client = stress.Client()
        self.insert_op = stress.InsertOp(self.key_generator, self.model.insert_chooser(), self.model, self.connection, values)
        self.client.add_op(insert_freq, self.insert_op)
        # TODO: Does this specific workload make sense?
        self.small_rget_op = stress.PercentageRangeReadOp(self.connection, percentage=(1,2), limit=small_rget_size )
        self.client.add_op(small_rget_freq, self.small_rget_op)
        self.large_rget_op = stress.PercentageRangeReadOp(self.connection, percentage=(30,100), limit=large_rget_size )
        self.client.add_op(large_rget_freq, self.large_rget_op)
    def poll_and_reset(self):
        queries = 0
        latencies = []
        for op in [self.insert_op, self.small_rget_op, self.large_rget_op]:
            op.lock()
            stats = op.poll()
            op.reset()
            op.unlock()
            queries += stats["queries"]
            # TODO: Differentiate between latencies from different operations
            latencies.extend(stats["latency_samples"])
            if op is self.insert_op: inserts = stats["queries"]
        return {"queries": queries, "latency_samples": latencies, "inserts": inserts}

clients = [RgetMixClient(i) for i in xrange(num_clients)]

# Start the clients

print "Running special rget-mix workload."
for c in clients: c.client.start()

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
    round_inserts = 0
    round_latencies = []
    for c in clients:
        stats = c.poll_and_reset()
        round_queries += stats["queries"]
        round_inserts += stats["inserts"]
        round_latencies.extend(stats["latency_samples"])

    # Write stats out
    if qps_file:
        qps_file.write("%d\t%d\n" % (total_time, round_queries))
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

