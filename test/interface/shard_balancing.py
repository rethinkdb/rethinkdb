#!/usr/bin/env python
# Copyright 2014 RethinkDB, all rights reserved.

"""The `interface.shard_balancing` test checks that RethinkDB generates balanced shards in a variety of scenarios."""

from __future__ import print_function

import pprint, os, sys, time

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
    proc1 = driver.Process(cluster1, files1, console_output="serve-output-1", command_prefix=command_prefix, extra_options=serve_options)
    files2 = driver.Files(metacluster, console_output="create-output-2", server_name="b", command_prefix=command_prefix)
    proc2 = driver.Process(cluster1, files2, console_output="serve-output-2", command_prefix=command_prefix, extra_options=serve_options)
    proc1.wait_until_started_up()
    proc2.wait_until_started_up()
    cluster1.check()
    conn = r.connect("localhost", proc1.driver_port)

    res = r.db_create("test").run(conn)
    assert res["created"] == 1

    print("Testing pre-sharding with UUID primary keys...")
    res = r.table_create("uuid_pkey").run(conn)
    assert res["created"] == 1
    r.table("uuid_pkey").reconfigure(shards=10, replicas=1).run(conn)
    r.table_wait("uuid_pkey").run(conn)
    res = r.table("uuid_pkey").insert([{}]*1000).run(conn)
    assert res["inserted"] == 1000 and res["errors"] == 0
    res = r.table("uuid_pkey").info().run(conn)["doc_count_estimates"]
    pprint.pprint(res)
    for num in res:
        assert 50 < num < 200

    print("Testing down-sharding existing balanced shards...")
    r.table("uuid_pkey").reconfigure(shards=2, replicas=1).run(conn)
    r.table_wait("uuid_pkey").run(conn)
    res = r.table("uuid_pkey").info().run(conn)["doc_count_estimates"]
    pprint.pprint(res)
    for num in res:
        assert 250 < num < 750

    print("Testing sharding of existing inserted data...")
    res = r.table_create("numeric_pkey").run(conn)
    assert res["created"] == 1
    res = r.table("numeric_pkey").insert([{"id": n} for n in xrange(1000)]).run(conn)
    assert res["inserted"] == 1000 and res["errors"] == 0
    r.table("numeric_pkey").reconfigure(shards=10, replicas=1).run(conn)
    r.table_wait("numeric_pkey").run(conn)
    res = r.table("numeric_pkey").info().run(conn)["doc_count_estimates"]
    pprint.pprint(res)
    for num in res:
        assert 50 < num < 200

    print("Creating an unbalanced table...")
    res = r.table_create("unbalanced").run(conn)
    assert res["created"] == 1
    r.table("unbalanced").reconfigure(shards=2, replicas=1).run(conn)
    r.table_wait("unbalanced").run(conn)
    res = r.table("unbalanced").insert([{"id": n} for n in xrange(1000)]).run(conn)
    assert res["inserted"] == 1000 and res["errors"] == 0
    res = r.table("unbalanced").info().run(conn)["doc_count_estimates"]
    pprint.pprint(res)
    assert res[0] > 500
    assert res[1] < 100

    # RSI(reql_admin): Once #2896 is implemented, make sure the server has an issue now

    print("Fixing the unbalanced table...")
    r.table("unbalanced").rebalance().run(conn)
    r.table_wait("unbalanced").run(conn)
    res = r.table("unbalanced").info().run(conn)["doc_count_estimates"]
    pprint.pprint(res)
    for num in res:
        assert 250 < num < 750

    cluster1.check_and_stop()
print("Done.")

