#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.
import os, subprocess, sys, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils
from vcoptparse import *
r = utils.import_python_driver()

op = OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

# This tests a specific feature of the ReQL tests: namely, the possibility of running
# them against a specific table. Since it's a meta-test it's not terribly important. The
# reason why this test exists is that in the reql_admin branch this feature is used to
# test the artificial table logic. So this test is sort of acting as a placeholder, to
# make sure the feature works properly until reql_admin is merged into next.
# - TM 2014-08-29

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    _, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    print "Spinning up a process..."
    files = driver.Files(metacluster, db_path="db", console_output="create-output", command_prefix=command_prefix)
    proc = driver.Process(cluster, files, console_output="serve-output", command_prefix=command_prefix, extra_options=serve_options)
    proc.wait_until_started_up()
    cluster.check()

    print "Creating a table..."
    conn = r.connect("localhost", proc.driver_port)
    res = r.db_create("test_db").run(conn)
    assert res == {"created":1}
    res = r.db("test_db").table_create("test_table").run(conn)
    assert res == {"created":1}
    conn.close()

    command_line = [
        os.path.abspath(os.path.join(
            os.path.dirname(__file__), os.path.pardir, 'rql_test', 'test-runner')),
        '--cluster-port', str(proc.cluster_port),
        '--driver-port', str(proc.driver_port),
        '--table', 'test_db.test_table']
    command_line.extend('polyglot/' + name for name in [
        'arraylimits', 'changefeeds/basic', 'control', 'geo/indexing', 'joins', 'match',
        'mutation/atomic_get_set', 'mutation/delete', 'mutation/insert',
        'mutation/replace', 'mutation/update', 'polymorphism'])
    command_line.extend('polyglot/regression/%d' % issue_number for issue_number in [
        309, 453, 522, 545, 568, 578, 579, 678, 1001, 1155, 1179, 1468, 1789, 2399, 2697,
        2709, 2767, 2774, 2838, 2930])
    print "Command line:", " ".join(command_line)
    print "Running the QL test..."
    with open("test-runner-log.txt", "w") as f:
        subprocess.check_call(command_line, stdout=f, stderr=f)
    print "QL test finished."

    cluster.check_and_stop()
print "Done."
