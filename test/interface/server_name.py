#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(
    os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, http_admin, scenario_common, utils
r = utils.import_pyton_driver()
from vcoptparse import *

op = OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = \
        scenario_common.parse_mode_flags(opts)
    print "Spinning up two processes..."
    files1 = driver.Files(
        metacluster, db_path = "db-1", log_path = "create-output-1", machine_name="a",
        executable_path = executable_path, command_prefix = command_prefix)
    files2 = driver.Files(
        metacluster, db_path = "db-2", log_path = "create_output-2", machine_name="b",
        executable_path = executable_path, command_prefix = command_prefix)
    process1 = driver.Process(cluster, files1, log_path="serve-output-1",
        executable_path = executable_path, command_prefix = command_prefix,
        extra_options = serve_options)
    time.sleep(3)
    process2 = driver.Process(cluster, files2, log_path="serve-output-2",
        executable_path = executable_path, command_prefix = command_prefix,
        extra_options = serve_options)
    process1.wait_until_started_up()
    process2.wait_until_started_up()
    cluster.check()
    http_access1 = http_admin.ClusterAccess([("localhost", process1.http_port)])
    reql_conn1 = r.connect("localhost", process1.driver_port)
    reql_conn2 = r.connect("localhost", process2.driver_port)
    http_machine1 = http_access1.find_machine("a")
    http_machine2 = http_access1.find_machine("b")
    def check_names(name1, name2):
        http_access1.update_cluster_data(10)
        assert http_machine1.name == name1
        assert http_machine2.name == name2

    print "Checking typical scenarios..."
    check_names("a", "b")
    r.server_rename("a", "a2").run(reql_conn1)
    check_names("a2", "b")
    r.server_rename("b", "b2").run(reql_conn1)
    check_names("a2", "b2")
    cluster.check()
    http_access1.assert_no_issues()

    print "Checking error scenarios..."
    try:
        r.server_rename("x", "y").run(reql_conn1)
    except r.RqlRuntimeError, e:
        pass
    else:
        assert False, "renaming nonexistent server should fail"
    try:
        r.server_rename("a2", "b2").run(reql_conn1)
    except r.RqlRuntimeError, e:
        pass
    else:
        assert False, "creating a name conflict should fail"
    cluster.check()
    http_access1.assert_no_issues()

    print "Creating a netsplit, then waiting 20s..."
    side_cluster = driver.Cluster(metacluster)
    metacluster.move_processes(cluster, side_cluster, [process2])
    time.sleep(20)

    print "Checking behavior during netsplit..."
    r.server_rename("a2", "x").run(reql_conn1)
    check_names("x", "b2")
    try:
        r.server_rename("b2", "b3").run(reql_conn1)
    except r.RqlRuntimeError, e:
        pass
    else:
        assert False, "renaming disconnected server should fail"
    r.server_rename("b2", "x").run(reql_conn2)
    cluster.check()
    side_cluster.check()

    print "Ending the netsplit, then waiting 10s..."
    metacluster.move_processes(side_cluster, cluster, [process2])
    time.sleep(10)
    cluster.check()
    http_access1.assert_no_issues()

    print "Checking that rename occurred..."
    http_access1.update_cluster_data(10)
    assert set([http_machine1.name, http_machine2.name]) == set(["x", "x_renamed"])

    cluster.check_and_stop()

