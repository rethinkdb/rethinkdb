#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

raise NotImplementedError('Needs Jobs table: https://github.com/rethinkdb/rethinkdb/issues/3115')

import os, sys, time

try:
    xrange
except NameError:
    xrange = range

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, http_admin, scenario_common, utils, vcoptparse

r = utils.import_python_driver()

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    print "Starting cluster..."
    processes = [
        driver.Process(
        	cluster,
            driver.Files(metacluster, console_output="create-output-%d" % i, command_prefix=command_prefix),
            console_output="serve-output-%d" % i, command_prefix=command_prefix, extra_options=serve_options)
        for i in xrange(2)]
    for process in processes:
        process.wait_until_started_up()
    print "Creating table..."
    http = http_admin.ClusterAccess([("localhost", p.http_port) for p in processes])
    dc = http.add_datacenter()
    for server_id in http.servers:
        http.move_server_to_datacenter(server_id, dc)
    ns = http.add_table(primary = dc)
    time.sleep(10)
    host, port = driver.get_table_host(processes)

    print "Performing test queries..."
    with r.connect(host, port) as conn:
        batch = []
        for i in range(10000):
            batch.append({'id': str(i) * 10, 'val': str(i) * 20})
            if (i + 1) % 100 == 0:
                r.table(ns.name).insert(batch).run(conn)
                batch = []
                print i + 1,
                sys.stdout.flush()
        print
    print "Done with test queries."

    print "Adding a replica."
    http.set_table_affinities(ns, {dc: 1})

    time.sleep(1)

    print "Checking backfill progress... ",
    progress = http.get_progress()
    for _, temp1 in progress.iteritems():
        for namespace_id, temp2 in temp1.iteritems():
            for activity_id, temp3 in temp2.iteritems():
                for region, progress_val in temp3.iteritems():
                    assert(progress_val[0] != "Timeout")
                    assert(progress_val[0][0] <= progress_val[0][1])
    print "OK."

    print "Shutting down cluster."
    cluster.check_and_stop()
