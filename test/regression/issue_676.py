#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

from __future__ import print_function

import sys, os, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin, driver

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'scenarios')))
import rdb_workload_common

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
    db = http.add_database(name="test")
    ns = http.add_table(primary=dc, name="test", primary_key="foo")
    time.sleep(10)
    host, port = driver.get_table_host(processes)
    cluster.check()

    rdb_workload_common.insert_many(host=host, port=port, table="test", count=10000)

    print("Splitting into two shards...")
    http.add_table_shard(ns, "t")
    time.sleep(10)
    cluster.check()

    print("Merging shards together again...")
    http.remove_table_shard(ns, "t")
    time.sleep(10)
    cluster.check()

    cluster.check_and_stop()
