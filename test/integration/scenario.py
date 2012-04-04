#!/usr/bin/python
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin
from workload_runner import WorkloadRunner
from vcoptparse import *

def get_default_option_parser():
	op = OptParser()
	op["workloads"] = ManyPositionalArgs()
	op["timeout"] = IntFlag("--timeout", 120)
	op["log-file"] = StringFlag("--log-file", "stdout")
	op["mode"] = StringFlag("--mode", "debug")
	return op

def run_scenario(opts, initialize_fn, phase_fns = []):
	# Take a random host/port from the cluster and run the workload with it
	cluster = http_admin.Cluster(opts["log-file"], opts["mode"])
	host, port = initialize_fn(cluster, opts)

	# Create workload objects - this will verify they exist and whatnot
	workloads = []
	for w in opts["workloads"]:
		workloads.append(WorkloadRunner(w))

	# TODO: run each combination of phases between scenario and workload
	# for now make sure that the number of workloads equals the number of phase functions + 1
	assert len(workloads) == len(phase_fns) + 1
	end_time = time.time() + opts["timeout"]

	workloads[0].run(host, port, opts["timeout"])

	for i in range(len(phase_fns)):
		# Advance the scenario
		if not cluster.is_alive():
			raise RuntimeError("Cluster crashed during workload")
		host, port = phase_fns[i]()
		if not cluster.is_alive():
			raise RuntimeError("Cluster crashed during phase break")

		# Run the next workload
		time_left = end_time - time.time()
		if time_left < 0:
			raise RuntimeError("Test timed out")
		workloads[i + 1].run(host, port, time_left)

	self.cluster = None

