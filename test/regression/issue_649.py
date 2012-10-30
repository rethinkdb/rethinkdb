# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/python
import sys, os, time, tempfile
rethinkdb_root = os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, os.path.pardir))
sys.path.append(os.path.join(rethinkdb_root, "test", "common"))
import http_admin, driver
from vcoptparse import *

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    print "Starting cluster..."
    num_nodes = 2
    files = [driver.Files(metacluster, db_path = "db-%d" % i, log_path = "create-output-%d" % i)
        for i in xrange(num_nodes)]
    processes = [driver.Process(
            cluster,
            files[i],
            log_path = "serve-output-%d" % i,
            executable_path = driver.find_rethinkdb_executable("debug"))
        for i in xrange(num_nodes)]
    time.sleep(10)
    print "Creating namespace..."
    http = http_admin.ClusterAccess([("localhost", p.http_port) for p in processes])
    dc = http.add_datacenter()
    for machine_id in http.machines:
        http.move_server_to_datacenter(machine_id, dc)
    ns = http.add_namespace(protocol = "memcached", primary = dc)
    time.sleep(10)
    cluster.check()

    print "Splitting into two shards..."
    http.add_namespace_shard(ns, "t")
    time.sleep(10)
    cluster.check()

    print "Increasing replication factor..."
    http.set_namespace_affinities(ns, {dc: 1})
    time.sleep(10)
    cluster.check()

    print "Merging shards together again..."
    http.remove_namespace_shard(ns, "t")
    time.sleep(10)
    cluster.check()

    cluster.check_and_stop()
