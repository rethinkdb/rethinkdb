#!/usr/bin/python
import sys, os, time, shlex, random
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin, driver, workload_runner, scenario_common
from vcoptparse import *

class Sequence(object):
    """A Sequence is a plan for a sequence of rebalancing operations. It
    consists of an initial number of shards and then a series of steps that
    remove one or more existing shard boundaries and add one or more new shard
    boundaries."""
    def __init__(self, initial, steps):
        assert isinstance(initial, int)
        for num_adds, num_removes in steps:
            assert isinstance(num_adds, int)
            assert isinstance(num_removes, int)
        self.initial = initial
        self.steps = steps
    @classmethod
    def from_string(cls, string):
        parts = string.split(",")
        initial = int(parts[0])
        steps = []
        for step in parts[1:]:
            assert set(step) <= set("+-")
            steps.append((step.count("+"), step.count("-")))
        return cls(initial, steps)

op = OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
workload_runner.prepare_option_parser_for_split_or_continuous_workload(op, allow_between = True)
op["num-nodes"] = IntFlag("--num-nodes", 3)
op["sequence"] = ValueFlag("--sequence", converter = Sequence.from_string, default = Sequence(2, [(1, 1)]))
opts = op.parse(sys.argv)

candidate_shard_boundaries = set("abcdefghijklmnopqrstuvwxyz")

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    print "Starting cluster..."
    processes = [driver.Process(cluster, driver.Files(metacluster, db_path = "db-%d" % i, executable_path = executable_path, command_prefix = command_prefix), log_path = "serve-output-%d" % i,
            executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
        for i in xrange(opts["num-nodes"])]
    for process in processes:
        process.wait_until_started_up()

    print "Creating namespace..."
    http = http_admin.ClusterAccess([("localhost", p.http_port) for p in processes])
    primary_dc = http.add_datacenter()
    secondary_dc = http.add_datacenter()
    machines = http.machines.keys()
    http.move_server_to_datacenter(machines[0], primary_dc)
    http.move_server_to_datacenter(machines[1], primary_dc)
    http.move_server_to_datacenter(machines[2], secondary_dc)
    ns = scenario_common.prepare_table_for_workload(opts, http, primary = primary_dc,
        affinities = {primary_dc.uuid: 1, secondary_dc.uuid: 1})
    shard_boundaries = set(random.sample(candidate_shard_boundaries, opts["sequence"].initial))
    print "Split points are:", list(shard_boundaries)
    http.change_namespace_shards(ns, adds = list(shard_boundaries))
    http.wait_until_blueprint_satisfied(ns)
    cluster.check()

    workload_ports = scenario_common.get_workload_ports(opts, ns, processes)
    with workload_runner.SplitOrContinuousWorkload(opts, opts["protocol"], workload_ports) as workload:
        workload.run_before()
        cluster.check()
        for i, (num_adds, num_removes) in enumerate(opts["sequence"].steps):
            if i != 0:
                workload.run_between()
            adds = set(random.sample(candidate_shard_boundaries - shard_boundaries, num_adds))
            removes = set(random.sample(shard_boundaries, num_removes))
            print "Splitting at", list(adds), "and merging at", list(removes)
            http.change_namespace_shards(ns, adds = list(adds), removes = list(removes))
            shard_boundaries = (shard_boundaries - removes) | adds
            http.wait_until_blueprint_satisfied(ns)
            cluster.check()
            http.check_no_issues()
        workload.run_after()

    cluster.check_and_stop()
