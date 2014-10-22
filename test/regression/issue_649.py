#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

from __future__ import print_function

import os, sys, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin, driver

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    print("Starting cluster...")
    num_nodes = 2
    files = [driver.Files(metacluster, db_path="db-%d" % i, log_path="create-output-%d" % i) for i in range(num_nodes)]
    processes = [driver.Process(cluster, files[i], log_path="serve-output-%d" % i) for i in range(num_nodes)]
    time.sleep(10)
    
    print("Creating table...")
    http = http_admin.ClusterAccess([("localhost", p.http_port) for p in processes])
    dc = http.add_datacenter()
    for machine_id in http.machines:
        http.move_server_to_datacenter(machine_id, dc)
    ns = http.add_table(primary=dc)
    time.sleep(10)
    cluster.check()

    print("Splitting into two shards...")
    http.add_table_shard(ns, "t")
    time.sleep(10)
    cluster.check()

    print("Increasing replication factor...")
    http.set_table_affinities(ns, {dc: 1})
    time.sleep(10)
    cluster.check()

    print("Merging shards together again...")
    http.remove_table_shard(ns, "t")
    time.sleep(10)
    cluster.check()

    cluster.check_and_stop()
