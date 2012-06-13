#!/usr/bin/python
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin, driver, workload_runner
from vcoptparse import *

op = OptParser()
workload_runner.prepare_option_parser_for_split_or_continuous_workload(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    print "Starting cluster..."
    cluster = driver.Cluster(metacluster)
    primary = driver.Process(cluster, driver.Files(metacluster, db_path = "db-1"), log_path = "serve-output-1")
    secondary = driver.Process(cluster, driver.Files(metacluster, db_path = "db-2"), log_path = "serve-output-2")
    primary.wait_until_started_up()
    secondary.wait_until_started_up()

    print "Creating namespace..."
    http = http_admin.ClusterAccess([("localhost", secondary.http_port)])
    primary_dc = http.add_datacenter()
    http.move_server_to_datacenter(primary.files.machine_name, primary_dc)
    secondary_dc = http.add_datacenter()
    http.move_server_to_datacenter(secondary.files.machine_name, secondary_dc)
    ns = http.add_namespace(protocol = "memcached", primary = primary_dc, affinities = {primary_dc: 0, secondary_dc: 1})
    http.set_namespace_ack_expectations(ns, {secondary_dc: 1})
    http.wait_until_blueprint_satisfied(ns)
    cluster.check()
    http.check_no_issues()

    host, port = driver.get_namespace_host(ns.port, [secondary])
    with workload_runner.SplitOrContinuousWorkload(opts, host, port) as workload:
        workload.step1()
        cluster.check()
        http.check_no_issues()
        print "Killing the primary..."
        primary.close()
        http.declare_machine_dead(primary.files.machine_name)
        http.move_namespace_to_datacenter(ns, secondary_dc)
        http.set_namespace_affinities(ns, {secondary_dc: 0})
        http.wait_until_blueprint_satisfied(ns)
        cluster.check()
        http.check_no_issues()
        workload.step2()

    http.check_no_issues()
    cluster.check_and_stop()
