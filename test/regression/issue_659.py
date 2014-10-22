#!/usr/bin/env python
# Copyright 2010-2012 RethinkDB, all rights reserved.

from __future__ import print_function

import sys, os, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin, driver, rdb_workload_common

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    print("Starting cluster...")
    num_nodes = 2
    files = [driver.Files(metacluster, db_path="db-%d" % i, console_output="create-output-%d" % i) for i in range(num_nodes)]
    processes = [driver.Process(cluster, files[i], console_output="serve-output-%d" % i) for i in range(num_nodes)]
    time.sleep(3)
    
    print("Creating table...")
    http = http_admin.ClusterAccess([("localhost", p.http_port) for p in processes])
    db = http.add_database("test")
    dc = http.add_datacenter()
    for machine_id in http.machines:
        http.move_server_to_datacenter(machine_id, dc)
    ns = http.add_table(primary=dc, name="stress", database=db)
    time.sleep(3)
    host, port = driver.get_table_host(processes)
    cluster.check()

    print("Increasing replication factor...")
    http.set_table_affinities(ns, {dc: 1})
    time.sleep(3)
    cluster.check()

    print("Inserting some data...")
    rdb_workload_common.insert_many(host=host, port=port, database="test", table="stress", count=20000)
    cluster.check()

    print("Decreasing replication factor...")
    http.set_table_affinities(ns, {dc: 0})
    time.sleep(3)
    cluster.check()

    print("Increasing replication factor again...")
    http.set_table_affinities(ns, {dc: 1})

    print("Confirming that the progress meter indicates a backfill happening...")
    for i in range(100):
        progress = http.get_progress()
        if len(progress) > 0:
            print("OK")
            break
        time.sleep(0.1)
    else:
        raise RuntimeError("Never detected a backfill happening")

    cluster.check()
    # Don't call `check_and_stop()` because it expects the server to shut down
    # in some reasonable period of time, but since the server has a lot of data
    # to flush to disk, it might not.
