#!/usr/bin/env python

import optparse, time, sys, random

documentation = """
The rget-mix workload cannot be expressed using the normal stress client, because it
contains range-gets of two different sizes. This script mimics the stress client enough
to be compatible with dbench. It accepts a subset of the parameters that the stress client
accepts, but its "workload" parameter must always be "-w special_rget_mix_workload" and
the other parts of the workload are hard-coded."""

parser = optparse.OptionParser(description = documentation)
parser.add_option("-c", "--clients", dest="clients", type="int", default=64,
    help="How many concurrent connections to make.", metavar="N")
parser.add_option("--client-suffix", dest="client_suffix", action="store_true", default=False,
    help="For compatibility with the stress client. Ignored.")
parser.add_option("-d", "--duration", dest="duration", default="10s",
    help="How long to run the test for. Should be a number with an 'i', 'q', or 's' after it "
    "to indicate that the duration is in inserts, queries, or seconds.", metavar="DURATION")
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
assert options.duration[-1] in "qis"
int(options.duration[:-1])   # Make sure it's a valid integer

# The rest of the workload parameters are hard-coded.

keys = (8,16)
values = (8,128)
insert_freq = 8
small_rget_freq = 2
small_rget_size = (8,128)
large_rget_freq = 1
large_rget_size = 100000

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
        self.client = stress.Client(i, options.clients, keys)
        self.connection = stress.Connection(options.servers[i % len(options.servers)])
        self.insert_op = stress.InsertOp(self.client, self.connection, insert_freq, values)
        # Temporary workaround so we can test this script even though the server crashes on
        # rgets.
        # self.small_rget_op = stress.RangeReadOp(self.client, self.connection, small_rget_freq, small_rget_size)
        # self.large_rget_op = stress.RangeReadOp(self.client, self.connection, large_rget_freq, large_rget_size)
        self.small_rget_op = stress.InsertOp(self.client, self.connection, insert_freq, values)
        self.large_rget_op = stress.InsertOp(self.client, self.connection, insert_freq, values)
    def poll_and_reset(self):
        self.client.lock()
        queries = 0
        latencies = []
        for op in [self.insert_op, self.small_rget_op, self.large_rget_op]:
            stats = op.poll()
            op.reset()
            queries += stats["queries"]
            # TODO: Differentiate between latencies from different operations
            latencies.extend(stats["latency_samples"])
            if op is self.insert_op: inserts = stats["queries"]
        self.client.unlock()
        return {"queries": queries, "latency_samples": latencies, "inserts": inserts}

clients = [RgetMixClient(i) for i in xrange(options.clients)]

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
    if {"q": total_queries, "i": total_inserts, "s": total_time}[options.duration[-1]] > int(options.duration[:-1]):
        break
