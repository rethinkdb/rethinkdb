#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

"""The `interface.table_doc_count_estimates` test checks that the `doc_count_estimates`
field on `r.table(...).info()` behaves as expected."""

import os, pprint, re, sys, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

r = utils.import_python_driver()

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster1 = driver.Cluster(metacluster)
    _, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    
    print("Spinning up two processes...")
    files1 = driver.Files(metacluster, console_output="create-output-1", server_name="a", command_prefix=command_prefix)
    proc1 = driver.Process(cluster1, files1, console_output = "serve-output-1", command_prefix=command_prefix, extra_options=serve_options)
    files2 = driver.Files(metacluster, console_output="create-output-2", server_name="b", command_prefix=command_prefix)
    proc2 = driver.Process(cluster1, files2, console_output="serve-output-2", command_prefix=command_prefix, extra_options=serve_options)
    proc1.wait_until_started_up()
    proc2.wait_until_started_up()
    cluster1.check()
    conn = r.connect("localhost", proc1.driver_port)

    res = r.db_create("test").run(conn)
    assert res["created"] == 1
    res = r.table_create("test").run(conn)
    assert res["created"] == 1

    res = r.table("test").info().run(conn)
    pprint.pprint(res)
    assert res["doc_count_estimates"] == [0]

    N = 100
    fudge = 2

    res = r.table("test").insert([{"id": "a%d" % i} for i in xrange(N)]).run(conn)
    assert res["inserted"] == N
    res = r.table("test").insert([{"id": "z%d" % i} for i in xrange(N)]).run(conn)
    assert res["inserted"] == N

    res = r.table("test").info().run(conn)
    pprint.pprint(res)
    assert N*2/fudge <= res["doc_count_estimates"][0] <= N*2*fudge

    r.table("test").reconfigure(shards=2, replicas=1).run(conn)
    res = list(r.table_wait("test").run(conn))
    pprint.pprint(res)

    res = r.table("test").info().run(conn)
    pprint.pprint(res)
    assert N/fudge <= res["doc_count_estimates"][0] <= N*fudge
    assert N/fudge <= res["doc_count_estimates"][1] <= N*fudge

    # Make sure that oversharding doesn't break distribution queries
    r.table("test").reconfigure(shards=2, replicas=2).run(conn)
    res = list(r.table_wait("test").run(conn))
    pprint.pprint(res)

    res = r.table("test").info().run(conn)
    pprint.pprint(res)
    assert N/fudge <= res["doc_count_estimates"][0] <= N*fudge
    assert N/fudge <= res["doc_count_estimates"][1] <= N*fudge

    res = r.table("test").filter(r.row["id"].split("").nth(0) == "a").delete().run(conn)
    assert res["deleted"] == N

    res = r.table("test").info().run(conn)
    pprint.pprint(res)
    assert res["doc_count_estimates"][0] <= N/fudge
    assert N/fudge <= res["doc_count_estimates"][1] <= N*fudge

    # Check to make sure that system tables work too
    res = r.db("rethinkdb").table("server_config").info().run(conn)
    pprint.pprint(res)
    assert res["doc_count_estimates"] == [2]

    cluster1.check_and_stop()
print("Done.")

