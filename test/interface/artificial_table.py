#!/usr/bin/env python
# Copyright 2014-2015 RethinkDB, all rights reserved.

'''This runs a bunch of the ReQL tests against the `rethinkdb._debug_scratch` artificial table to check that `artificial_table_t` works properly.'''

from __future__ import print_function

import os, subprocess, sys, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

r = utils.import_python_driver()

print("Spinning up a server (%.2fs)" % (time.time() - startTime))
with driver.Process(files='db', output_folder='.', command_prefix=command_prefix, extra_options=serve_options, wait_until_ready=True) as server:
    server.check()
    
    conn = r.connect(host=server.host, port=server.driver_port)
    
    # note: these tests are impicitly run in sequence because of the --table option
    
    command_line = [
        os.path.join(os.path.dirname(os.path.abspath(__file__)), os.path.pardir, 'rql_test', 'test-runner'),
        '--cluster-port', str(server.cluster_port),
        '--driver-port', str(server.driver_port),
        '--table', 'rethinkdb._debug_scratch']
    command_line.extend('polyglot/' + name for name in [
        'control', 'joins', 'match',
        'mutation/atomic_get_set', 'mutation/delete', 'mutation/insert',
        'mutation/replace', 'mutation/update', 'polymorphism'])
    command_line.extend('polyglot/regression/%d' % issue_number for issue_number in [
        309, 453, 522, 545, 568, 678, 1155, 1179, 1468, 2399, 2697,
        2709, 2838, 2930])
    
    print('Ensuring that db "test" exists for secondary table (%.2fs)' % (time.time() - startTime))
    if 'test' not in r.db_list().run(conn):
        r.db_create('test').run(conn)
    
    print("Command line:", " ".join(command_line))
    print("Running the QL test (%.2fs)" % (time.time() - startTime))

    print('==== Start test-runner Output ====')
    subprocess.check_call(command_line)
    print('==== End test-runner Output ====')
    
    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))
