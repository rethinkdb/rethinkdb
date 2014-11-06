#!/usr/bin/env python
# Copyright 2014 RethinkDB, all rights reserved.

from __future__ import print_function

import os, subprocess, sys, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

r = utils.import_python_driver()

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

# This runs a bunch of the ReQL tests against the `rethinkdb._debug_scratch` artificial
# table to check that `artificial_table_t` works properly.

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    _, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    
    print("Spinning up a process...")
    files = driver.Files(metacluster, db_path="db", console_output="create-output", command_prefix=command_prefix)
    proc = driver.Process(cluster, files, console_output="serve-output", command_prefix=command_prefix, extra_options=serve_options)
    proc.wait_until_started_up()
    cluster.check()

    command_line = [
        os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'rql_test', 'test-runner')),
        '--cluster-port', str(proc.cluster_port),
        '--driver-port', str(proc.driver_port),
        '--table', 'rethinkdb._debug_scratch']
    command_line.extend('polyglot/' + name for name in [
        'control', 'joins', 'match',
        'mutation/atomic_get_set', 'mutation/delete', 'mutation/insert',
        'mutation/replace', 'mutation/update', 'polymorphism'])
    command_line.extend('polyglot/regression/%d' % issue_number for issue_number in [
        309, 453, 522, 545, 568, 678, 1155, 1179, 1468, 2399, 2697,
        2709, 2838, 2930])
    print("Command line:", " ".join(command_line))
    print("Running the QL test...")
    with open("test-runner-log.txt", "w") as f:
        subprocess.check_call(command_line, stdout=f, stderr=f)
    print("QL test finished.")

    cluster.check_and_stop()
print("Done.")
