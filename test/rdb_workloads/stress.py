#!/usr/bin/python
import sys, os, time, signal, random, string, subprocess
from tempfile import NamedTemporaryFile
from optparse import OptionParser

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'drivers', 'python')))
import rethinkdb as r

client_script = os.path.join(os.path.dirname(__file__), "stress_client.py")

parser = OptionParser()
parser.add_option("--table", dest="db_table", metavar="DB.TABLE", default="", type="string")
parser.add_option("--timeout", dest="timeout", metavar="SECONDS", default=60, type="int")
parser.add_option("--clients", dest="clients", metavar="CLIENTS", default=64, type="int")
parser.add_option("--batch-size", dest="batch_size", metavar="BATCH_SIZE", default=100, type="int")
parser.add_option("--workload", dest="workload", metavar="WRITES/DELETES/READS/SINDEX_READS", default="3/2/5/0", type="string")
parser.add_option("--host", dest="hosts", metavar="HOST:PORT", action="append", default=[], type="string")
parser.add_option("--sindex", dest="sindexes", metavar="constant | simple | complex | long", action="append", default=[], type="string")
(options, args) = parser.parse_args()

if len(args) != 0:
    raise RuntimeError("No positional arguments supported")

# Parse out host/port pairs
hosts = [ ]
for host_port in options.hosts:
    (host, port) = host_port.split(":")
    hosts.append((host, int(port)))
if len(hosts) == 0:
    raise RuntimeError("No rethinkdb host specified")

# Parse out and verify secondary indexes
sindexes = [ ]
for sindex in options.sindexes:
    if sindex not in ["constant", "simple", "complex", "long"]:
        raise RuntimeError("sindex type not recognized: " + sindex)
    sindexes.append(sindex)

# Parse out workload info - probably an easier way to do this
workload = { }
workload_defaults = [("--writes", 3), ("--deletes", 2), ("--reads", 5), ("--sindex-reads", 0)]
workload_types = [item[0] for item in workload_defaults]
workload_values = options.workload.split("/")
if len(workload_values) < len(workload_types):
    raise RuntimeError("Too many workload values specified")
for op, value in zip(workload_types, workload_values):
    workload[op] = value
for op, value in workload_defaults:
    if op not in workload.keys():
        workload[op] = value

clients = [ ]
output_files = [ ]

def collect_and_print_results():
    global output_files

    # Read in each file so that we have a per-client array containing a
    #  dict of timestamps to dicts of op-names: op-counts
    results_per_client = [ ]
    for f in output_files:
        file_data = { }
        for line in f:
            split_line = line.strip().split(",")
            op_counts = { }
            timestamp = float(split_line[0])
            for (op_name, count) in zip(split_line[1::2], split_line[2::2]):
                op_counts[op_name] = int(count)
            file_data[timestamp] = op_counts
        results_per_client.append(file_data)

    # Until we do some real analysis on the results, just get ops/sec for each client
    averages = [ ]
    durations = [ ]
    for results in results_per_client:
        if len(results) < 2:
            print "Ignoring client result, not enough data points"
        else:
            keys = sorted(results.keys())
            duration = keys[-1] - keys[0]
            accumulator = { }
            for (timestamp, counts) in results.items():
                accumulator = dict((op, accumulator.get(op, 0) + counts.get(op, 0)) for op in set(accumulator) | set(counts))
            averages.append(dict((op, accumulator.get(op, 0) / (duration)) for op in accumulator.keys()))
            durations.append(duration)

    # Add up all the client averages for the total ops/sec
    total = { }
    for average in averages:
        total = dict((op, total.get(op, 0) + average.get(op, 0)) for op in set(total) | set(average))

    if len(durations) < 1:
        print "Not enough data for results"
    else:
        print "Duration: " + str(int(max(durations))) + " seconds"
        print "Operations per second: "
        for op in total.keys():
            print "  %s: %d" % (op, int(total[op]))

def finish_stress():
    global clients
    print "Stopping client processes..."
    for client in clients:
        client.send_signal(signal.SIGINT)

    # Wait up to 5s for clients to exit
    end_time = time.time() + 5
    while len(clients) > 0 and time.time() < end_time:
        time.sleep(0.1)
        clients = [client for client in clients if client.poll() is None]

    # Kill any remaining clients
    for client in clients:
        print "Client didn't stop in time, killing..."
        client.terminate()

    collect_and_print_results()

def interrupt_handler(signal, frame):
    print "Interrupted"
    finish_stress()
    exit(0)

# Get table name, and make sure it exists on the server
if len(options.db_table) == 0:
    # Create a new table
    random.seed()
    table = "stress_" + "".join(random.sample(string.letters + string.digits, 10))
    db = "test"

    with r.connect(hosts[0][0], hosts[0][1]) as connection:
        if db not in r.db_list().run(connection):
            r.db_create(db).run(connection)

        while table in r.db(db).table_list().run(connection):
            table = "stress_" + "".join(random.sample(string.letters + string.digits, 10))

        r.db(db).table_create(table).run(connection)
else:
    # Load an existing table
    if "." not in options.db_table:
        raise RuntimeError("Incorrect db.table format in --table option")

    (db, table) = options.db_table.split(".")

    with r.connect(hosts[0][0], hosts[0][1]) as connection:
        if db not in r.db_list().run(connection):
            r.db_create(db).run(connection)

        if table not in r.db(db).table_list().run(connection):
            r.db(db).table_create(table).run(connection)

        # TODO: load existing keys, distribute them among clients
        # TODO: fill out keys so that all keys are contiguous (for use by clients) - may be tricky

# Build up arg list for client processes
client_args = [client_script]
for (op, value) in workload.items():
    client_args.extend([op, value])
for sindex in sindexes:
    client_args.extend(["--sindex", sindex])
client_args.extend(["--batch-size", str(options.batch_size)])
client_args.extend(["--table", db + "." + table])

# Register interrupt, now that we're spawning client processes
signal.signal(signal.SIGINT, interrupt_handler)

# Launch all the client processes
for i in range(options.clients):
    output_file = NamedTemporaryFile()
    host, port = hosts[i % len(hosts)]

    current_args = list(client_args)
    current_args.extend(["--host", host + ":" + str(port)])
    current_args.extend(["--output", output_file.name])

    client = subprocess.Popen(current_args, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    clients.append(client)
    output_files.append(output_file)

print "Waiting for clients to connect..."
for client in clients:
    if client.stdout.readline().strip() != "ready":
        raise RuntimeError("unexpected client output")

print "Starting traffic..."
for client in clients:
    client.stdin.write("go\n")
    client.stdin.flush()

# Wait for timeout or interrupt
time.sleep(options.timeout)

finish_stress()

