#!/usr/bin/env python
# Copyright 2014-2015 RethinkDB, all rights reserved.

import datetime, os, socket, sys, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)

r = utils.import_python_driver()

utils.print_with_time("Spinning up two servers")
with driver.Cluster(output_folder='.') as cluster:
    
    process1 = driver.Process(cluster, name='a', server_tags=["foo"], command_prefix=command_prefix, extra_options=serve_options)
    process2 = driver.Process(cluster, name='b', server_tags=["foo", "bar"], command_prefix=command_prefix, extra_options=serve_options + ["--cache-size", "123"])
    
    cluster.wait_until_ready()
    
    utils.print_with_time("Establishing ReQL connection")
    
    conn = r.connect(process1.host, process1.driver_port)
    
    # -- general assertions
    
    utils.print_with_time("General status")
    
    assert r.db("rethinkdb").table("server_status").count().run(conn) == 2
    assert set(r.db("rethinkdb").table("server_status")["name"].run(conn)) == set(["a", "b"])

    # -- insert nonsense

    utils.print_with_time("Making sure that server_status is not writable")
    res = r.db("rethinkdb").table("server_status").delete().run(conn)
    assert res["errors"] == 2
    res = r.db("rethinkdb").table("server_status").update({"foo": "bar"}).run(conn)
    assert res["errors"] == 2
    res = r.db("rethinkdb").table("server_status").insert({}).run(conn)
    assert res["errors"] == 1

    # -- server a
    
    utils.print_with_time("First server info")
    
    st = r.db("rethinkdb").table("server_status").filter({"name":process1.name}).nth(0).run(conn)

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
    assert st["network"]["time_connected"] <= now
    assert st["network"]["time_connected"] >= st["process"]["time_started"]
    
    # -- server b
    
    utils.print_with_time("Second server info")
    
    st2 = r.db("rethinkdb").table("server_status").filter({"name":process2.name}).nth(0).run(conn)
    assert st2["process"]["cache_size_mb"] == 123
    
    assert st2["id"] == process2.uuid
    assert st2["process"]["pid"] == process2.process.pid
    
    assert st2["network"]["reql_port"] == process2.driver_port
    assert st2["network"]["cluster_port"] == process2.cluster_port
    assert st2["network"]["http_admin_port"] == process2.http_port
    assert st2["network"]["canonical_addresses"][0]["port"] == process2.cluster_port
    
    # -- shut down server b
    
    utils.print_with_time("Shutting down second server")
    
    process2.check_and_stop()
    
    deadline = time.time() + 10
    while time.time() < deadline:
        st2 = list(r.db("rethinkdb").table("server_status").filter({"name":process2.name}).run(conn))
        if st2 == []:
            break
    else:
        assert False, 'Server b did not become unavalible after 10 seconds'
    
    utils.print_with_time("Cleaning up")
utils.print_with_time("Done.")
