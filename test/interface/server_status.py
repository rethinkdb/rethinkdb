#!/usr/bin/env python
# Copyright 2014 RethinkDB, all rights reserved.

from __future__ import print_function

import copy, datetime, os, pprint, socket, sys, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)

r = utils.import_python_driver()

print("Spinning up two servers (%.2fs)" % (time.time() - startTime))
with driver.Cluster(output_folder='.') as cluster:
    
    process1 = driver.Process(cluster, files='a', server_tags=["foo"], command_prefix=command_prefix, extra_options=serve_options)
    process2 = driver.Process(cluster, files='b', server_tags=["foo", "bar"], command_prefix=command_prefix, extra_options=serve_options + ["--cache-size", "123"])
    
    cluster.wait_until_ready()
    
    print("Establishing ReQL connection (%.2fs)" % (time.time() - startTime))
    
    conn = r.connect(process1.host, process1.driver_port)
    
    # -- general assertions
    
    print("General status (%.2fs)" % (time.time() - startTime))
    
    assert r.db("rethinkdb").table("server_status").count().run(conn) == 2
    assert set(r.db("rethinkdb").table("server_status")["name"].run(conn)) == set(["a", "b"])
    
    # -- server a
    
    print("First server info (%.2fs)" % (time.time() - startTime))
    
    st = r.db("rethinkdb").table("server_status").filter({"name":process1.name}).nth(0).run(conn)
    
    #print("Status for a connected server:")
    #pprint.pprint(st)
    
    assert st["status"] == "connected"

    assert isinstance(st["process"]["version"], basestring)
    assert st["process"]["version"].startswith("rethinkdb")

    assert st["id"] == process1.uuid
    assert st["process"]["pid"] == process1.process.pid

    assert st["network"]["hostname"] == socket.gethostname()
    assert st["network"]["reql_port"] == process1.driver_port
    assert st["network"]["cluster_port"] == process1.cluster_port
    assert st["network"]["http_admin_port"] == process1.http_port
    assert st["network"]["canonical_addresses"][0]["port"] == process1.cluster_port

    now = datetime.datetime.now(st["process"]["time_started"].tzinfo)
    assert st["process"]["time_started"] <= now
    assert st["process"]["time_started"] > now - datetime.timedelta(minutes=1)
    assert st["connection"]["time_connected"] <= now
    assert st["connection"]["time_connected"] >= st["process"]["time_started"]
    assert st["connection"]["time_disconnected"] is None
    
    # -- server b
    
    print("Second server info (%.2fs)" % (time.time() - startTime))
    
    st2 = r.db("rethinkdb").table("server_status").filter({"name":process2.name}).nth(0).run(conn)
    assert st2["process"]["cache_size_mb"] == 123
    assert st2["status"] == "connected"
    
    assert st2["id"] == process2.uuid
    assert st2["process"]["pid"] == process2.process.pid
    
    assert st2["network"]["reql_port"] == process2.driver_port
    assert st2["network"]["cluster_port"] == process2.cluster_port
    assert st2["network"]["http_admin_port"] == process2.http_port
    assert st2["network"]["canonical_addresses"][0]["port"] == process2.cluster_port
    
    # -- shut down server b
    
    print("Shutting down second server (%.2fs)" % (time.time() - startTime))
    
    process2.check_and_stop()
    
    deadline = time.time() + 10
    while time.time() < deadline:
        st2 = r.db("rethinkdb").table("server_status").filter({"name":process2.name}).nth(0).run(conn)
        if st2["status"] == "disconnected":
            break
    else:
        assert False, 'Server b did not become unavalible after 10 seconds'
    
    #print("Status for a disconnected server:")
    #pprint.pprint(st2)

    now = datetime.datetime.now(st2["connection"]["time_disconnected"].tzinfo)
    assert st2["connection"]["time_connected"] is None
    assert st2["connection"]["time_disconnected"] <= now
    assert st2["connection"]["time_disconnected"] >= now - datetime.timedelta(minutes=1)
    
    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))
