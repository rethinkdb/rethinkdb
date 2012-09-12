#!/usr/bin/python
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin, driver, workload_runner, scenario_common
from vcoptparse import *

class ReplicaSequence(object):
    def __init__(self, string):
        assert set(string) < set("0123456789+-")
        string = string.replace("-", "\0-").replace("+", "\0+")
        parts = string.split("\0")
        assert set(parts[0]) < set("0123456789")
        self.initial = int(parts[0])
        self.steps = [int(x) for x in parts[1:]]
        current = self.initial
        for step in self.steps:
            current += step
            assert current >= 0
    def peak(self):
        peak = current = self.initial
        for step in self.steps:
            current += step
            peak = max(current, peak)
        return peak
    def __repr__(self):
        return str(self.initial) + "".join(("+" if s > 0 else "-") + str(abs(x)) for x in self.steps)

op = OptParser()
workload_runner.prepare_option_parser_for_split_or_continuous_workload(op, allow_between = True)
scenario_common.prepare_option_parser_mode_flags(op)
op["sequence"] = PositionalArg(converter = ReplicaSequence)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    print "Starting cluster..."
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    primary_process = driver.Process(cluster, driver.Files(metacluster, db_path = "db-primary", executable_path = executable_path, command_prefix = command_prefix), log_path = "serve-output-primary",
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
    replica_processes = [driver.Process(cluster, driver.Files(metacluster, db_path = "db-%d" % i, executable_path = executable_path, command_prefix = command_prefix), log_path = "serve_output-%d" % i,
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
        for i in xrange(opts["sequence"].peak())]
    primary_process.wait_until_started_up()
    for replica_process in replica_processes:
        replica_process.wait_until_started_up()

    print "Creating namespace..."
    http = http_admin.ClusterAccess([("localhost", p.http_port) for p in [primary_process] + replica_processes])
    primary_dc = http.add_datacenter()
    http.move_server_to_datacenter(primary_process.files.machine_name, primary_dc)
    replica_dc = http.add_datacenter()
    for replica_process in replica_processes:
        http.move_server_to_datacenter(replica_process.files.machine_name, replica_dc)
    ns = scenario_common.prepare_table_for_workload(opts, http, primary = primary_dc, affinities = {primary_dc: 0, replica_dc: opts["sequence"].initial})
    http.wait_until_blueprint_satisfied(ns)
    cluster.check()
    http.check_no_issues()

    workload_ports = scenario_common.get_workload_ports(opts, ns, [primary_process] + replica_processes)
    with workload_runner.SplitOrContinuousWorkload(opts, opts["protocol"], workload_ports) as workload:
        workload.run_before()
        cluster.check()
        http.check_no_issues()
        workload.check()
        current = opts["sequence"].initial
        for i, s in enumerate(opts["sequence"].steps):
            if i != 0:
                workload.run_between()
            print "Changing the number of secondaries from %d to %d..." % (current, current + s)
            current += s
            http.set_namespace_affinities(ns, {primary_dc: 0, replica_dc: current})
            http.wait_until_blueprint_satisfied(ns, timeout = 3600)
            cluster.check()
            http.check_no_issues()
        workload.run_after()

    http.check_no_issues()
    cluster.check_and_stop()
