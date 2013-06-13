#!/usr/bin/python
import sys, os, time, signal, random, string, subprocess
from tempfile import NamedTemporaryFile
from optparse import OptionParser

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'drivers', 'python')))
import rethinkdb as r

client_script = os.path.join(os.path.dirname(__file__), "stress_client.py")

parser = OptionParser()
parser.add_option("--table", dest="db_table", metavar="DB.TABLE", default="", type="string")
parser.add_option("--timeout", dest="timeout", metavar="SECONDS", default=60, type="int")
parser.add_option("--clients", dest="clients", metavar="CLIENTS", default=64, type="int")
parser.add_option("--batch-size", dest="batch_size", metavar="BATCH_SIZE", default=100, type="int")
parser.add_option("--workload", dest="workload", metavar="WRITES/DELETES/READS/SINDEX_READS/UPDATES/NON_ATOMIC_UPDATES", default="3/2/5/0/1/1", type="string")
parser.add_option("--host", dest="hosts", metavar="HOST:PORT", action="append", default=[], type="string")
parser.add_option("--add-sindex", dest="sindexes", metavar="constant | simple | complex | long", action="append", default=[], type="string")
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
workload_defaults = [("--writes", 3),
                     ("--deletes", 2),
                     ("--reads", 5),
                     ("--sindex-reads", 0),
                     ("--updates", 1),
                     ("--non-atomic-updates", 1)]
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
    # Format is "<time>[,<op_type>,<op_count>,<op_errs>,<avg_duration>]...
    results_per_client = [ ]
    errors = { }
    for f in output_files:
        file_data = { }
        for line in f:
            split_line = line.strip().split(",")
            op_counts = { }
            op_durations = { }
            if split_line[0] == "ERROR":
                key = split_line[1].strip()
                errors[key] = errors.get(key, 0) + 1
            else:
                timestamp = float(split_line[0])
                for (op_name, op_count, err_count, avg_dur) in zip(split_line[1::4], split_line[2::4], split_line[3::4], split_line[4::4]):
                    op_counts[op_name] = (int(op_count), int(err_count))
                    op_durations[op_name] = int(float(avg_dur) * 1000)
                file_data[timestamp] = op_counts
        results_per_client.append(file_data)

    # Until we do some real analysis on the results, just get ops/sec for each client
    total_per_client = [ ]
    averages = [ ]
    durations = [ ]
    ignored_results = 0
    for results in results_per_client:
        if len(results) < 2:
            ignored_results += 1
        else:
            keys = sorted(results.keys())
            duration = keys[-1] - keys[0]
            accumulator = { }
            for (timestamp, counts) in results.items():
                accumulator = dict((op, map(sum, zip(accumulator.get(op, (0, 0)), counts.get(op, (0, 0))))) for op in set(accumulator) | set(counts))
            total_per_client.append(accumulator)
            averages.append(dict((op, accumulator.get(op, (0, 0))[0] / (duration)) for op in accumulator.keys()))
            durations.append(duration)

    if ignored_results > 0:
        print "Ignored %d client results due to insufficient data" % ignored_results

    # Get the total number of ops of each type
    total_op_counts = { }
    for client_data in total_per_client:
        total_op_counts = dict((op, map(sum, zip(total_op_counts.get(op, (0, 0)), client_data.get(op, (0, 0))))) for op in set(client_data) | set(total_op_counts))

    # Add up all the client averages for the total ops/sec
    total = { }
    min_ops_per_sec = { }
    max_ops_per_sec = { }
    for average in averages:
        total = dict((op, total.get(op, 0) + average.get(op, 0)) for op in set(total) | set(average))
        # Get the lowest and highest per-client ops/sec
        min_ops_per_sec = dict((op, min(min_ops_per_sec.get(op, 10000000), average.get(op))) for op in set(min_ops_per_sec) | set(average))
        max_ops_per_sec = dict((op, max(max_ops_per_sec.get(op, 0), average.get(op))) for op in set(max_ops_per_sec) | set(average))

    if len(durations) < 1:
        print "Not enough data for results"
    else:
        print "Duration: " + str(int(max(durations))) + " seconds"
        print "\nOperations data: "

        table = [["op type", "successes", "per sec min", "per sec max", "per sec total", "errors", "avg duration"]]
        for op in total.keys():
            table.append([op, str(total_op_counts[op][0]), str(int(min_ops_per_sec[op])), str(int(max_ops_per_sec[op])), str(int(total[op])), str(total_op_counts[op][1]), "-"])

        column_widths = []
        for i in range(len(table[0])):
            column_widths.append(max([len(row[i]) + 2 for row in table]))

        format_str = ("{:<%d}" + ("{:>%d}" * (len(column_widths) - 1))) % tuple(column_widths)

        for row in table:
            print format_str.format(*row)

    if len(errors) != 0:
        print "\nErrors encountered:"
        for error in errors:
            print "%s: %s" % (error, errors[error])

def finish_stress():
    global clients
    print "Stopping client processes..."
    [client.send_signal(signal.SIGINT) for client in clients if client.poll() is None]

    # Wait up to 5s for clients to exit
    end_time = time.time() + 5
    while len(clients) > 0 and time.time() < end_time:
        time.sleep(0.1)
        clients = [client for client in clients if client.poll() is None]

    # Kill any remaining clients
    [client.terminate() for client in clients]

    collect_and_print_results()

def interrupt_handler(signal, frame):
    print "Interrupted"
    finish_stress()
    exit(0)

def complex_sindex_fn(row, db, table):
    return r.expr([row["value"]]).concat_map(lambda item: [item, item, item, item]) \
                                 .concat_map(lambda item: [item, item, item, item]) \
                                 .concat_map(lambda item: [item, item, item, item]) \
                                 .concat_map(lambda item: [item, item, item, item]) \
                                 .concat_map(lambda item: [item, item, item, item]) \
                                 .concat_map(lambda item: [item, item, item, item]) \
                                 .reduce(lambda acc, val: acc + val, 0)
    return 1

def long_sindex_fn(row):
    result = []
    for i in range(32):
        denom = 2 ** i
        result.insert(0, r.branch(((row["value"] / denom) % 2) == 0, "zero", "one"))
    return result

def initialize_sindexes(sindexes, connection, db, table):
    # This assumes sindexes are never deleted
    #  if they are and a table is loaded, there could be problems
    sindex_count = len(r.db(db).table(table).index_list().run(connection))
    for sindex in sindexes:
        # Sindexes are named as their type of sindex (below) plus a unique number
        sindex_name = sindex + str(sindex_count)
        sindex_count += 1
        sindex_fn = None
        if sindex == "constant":
            sindex_fn = lambda x: 1
        elif sindex == "simple":
            sindex_fn = lambda x: r.branch(x["value"] % 2 == 0, "odd", "even")
        elif sindex == "complex":
            sindex_fn = lambda x: complex_sindex_fn(x, db, table)
        elif sindex == "long":
            sindex_fn = long_sindex_fn
        else:
            raise RuntimeError("Unknown sindex type")
        print "Adding sindex '%s'..." % sindex_name
        r.db(db).table(table).index_create(sindex_name, sindex_fn).run(connection)

# Get table name, and make sure it exists on the server
if len(options.db_table) == 0:
    print "Creating table..."
    random.seed()
    table = "stress_" + "".join(random.sample(string.letters + string.digits, 10))
    db = "test"

    with r.connect(hosts[0][0], hosts[0][1]) as connection:
        if db not in r.db_list().run(connection):
            r.db_create(db).run(connection)

        while table in r.db(db).table_list().run(connection):
            table = "stress_" + "".join(random.sample(string.letters + string.digits, 10))

        r.db(db).table_create(table).run(connection)
        initialize_sindexes(sindexes, connection, db, table)
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

        initialize_sindexes(sindexes, connection, db, table)
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

print "Launching client processes..."
count_width = len(str(options.clients))
progress_format = "\r[%%%dd/%%%dd]" % (count_width, count_width)
done_format = "\r[%%%ds]" % (count_width * 2 + 1) % "DONE"

# Launch all the client processes
for i in range(options.clients):
    print (progress_format % (i, options.clients)),
    sys.stdout.flush()
    output_file = NamedTemporaryFile()
    host, port = hosts[i % len(hosts)]

    current_args = list(client_args)
    current_args.extend(["--host", host + ":" + str(port)])
    current_args.extend(["--output", output_file.name])

    client = subprocess.Popen(current_args, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    clients.append(client)
    output_files.append(output_file)
print done_format

print "Waiting for clients to connect..."
for i in range(options.clients):
    print (progress_format % (i, options.clients)),
    sys.stdout.flush()
    if clients[i].stdout.readline().strip() != "ready":
        raise RuntimeError("unexpected client output")
print done_format

print "Running traffic..."
for client in clients:
    client.stdin.write("go\n")
    client.stdin.flush()

# Wait for timeout or interrupt
end_time = time.time() + options.timeout
while time.time() < end_time:
    time.sleep(1)
    # Check to see if all the clients have exited (perhaps due to the cluster going down)
    if not any([client.poll() == None for client in clients]):
        print "All clients have exited prematurely"
        break

finish_stress()

