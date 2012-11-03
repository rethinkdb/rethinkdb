#!/usr/bin/python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import sys, os, time, tempfile, subprocess
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
    time.sleep(3)
    print "Creating namespace..."
    http = http_admin.ClusterAccess([("localhost", p.http_port) for p in processes])
    dc = http.add_datacenter()
    for machine_id in http.machines:
        http.move_server_to_datacenter(machine_id, dc)
    ns = http.add_namespace(protocol = "memcached", primary = dc)
    time.sleep(3)
    host, port = driver.get_namespace_host(ns.port, processes)
    cluster.check()

    print "Increasing replication factor..."
    http.set_namespace_affinities(ns, {dc: 1})
    time.sleep(3)
    cluster.check()

    print "Inserting some data..."
    subprocess.check_call(["%s/bench/stress-client/stress" % rethinkdb_root, "-w", "0/0/1/0", "-d", "20000q", "-s", "sockmemcached,%s:%d" % (host, port)])
    cluster.check()

    print "Decreasing replication factor..."
    http.set_namespace_affinities(ns, {dc: 0})
    time.sleep(3)
    cluster.check()

    print "Increasing replication factor again..."
    http.set_namespace_affinities(ns, {dc: 1})

    print "Confirming that the progress meter indicates a backfill happening..."
    for i in xrange(100):
        progress = http.get_progress()
        if len(progress) > 0:
            print "OK"
            break
        time.sleep(0.1)
    else:
        raise RuntimeError("Never detected a backfill happening")

    cluster.check()
    # Don't call `check_and_stop()` because it expects the server to shut down
    # in some reasonable period of time, but since the server has a lot of data
    # to flush to disk, it might not.
