#!/usr/bin/env python
# Copyright 2014-2016 RethinkDB, all rights reserved.

"""Checks that RethinkDB generates balanced shards in a variety of scenarios."""

import os, pprint, sys, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

try:
    xrange
except NameError:
    xrange = range

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()

utils.print_with_time("Spinning up two servers")
with driver.Cluster(initial_servers=['a', 'b'], output_folder='.', command_prefix=command_prefix, extra_options=serve_options, wait_until_ready=True) as cluster:
    cluster.check()
    
    utils.print_with_time("Establishing ReQL connection")
    
    conn = r.connect(host=cluster[0].host, port=cluster[0].driver_port)
    conn2 = r.connect(host=cluster[1].host, port=cluster[1].driver_port)
    
    utils.print_with_time("Creating db if necessary")
    
    if not dbName in r.db_list().run(conn):
        r.db_create(dbName).run(conn)

    utils.print_with_time("Testing pre-sharding with UUID primary keys")
    res = r.db(dbName).table_create("uuid_pkey").run(conn)
    assert res["tables_created"] == 1
    r.db(dbName).table("uuid_pkey").reconfigure(shards=10, replicas=1).run(conn)
    r.db(dbName).table("uuid_pkey").wait(wait_for="all_replicas_ready").run(conn)
    res = r.db(dbName).table("uuid_pkey").insert([{}]*1000).run(conn)
    assert res["inserted"] == 1000 and res["errors"] == 0
    res = r.db(dbName).table("uuid_pkey").info().run(conn)["doc_count_estimates"]
    pprint.pprint(res)
    for num in res:
        assert 50 < num < 200

    utils.print_with_time("Testing down-sharding existing balanced shards")
    r.db(dbName).table("uuid_pkey").reconfigure(shards=2, replicas=1).run(conn)
    r.db(dbName).table("uuid_pkey").wait(wait_for="all_replicas_ready").run(conn)
    res = r.db(dbName).table("uuid_pkey").info().run(conn)["doc_count_estimates"]
    pprint.pprint(res)
    for num in res:
        assert 250 < num < 750

    utils.print_with_time("Checking shard balancing by reading directly")
    direct_counts = [
        r.db(dbName).table("uuid_pkey", read_mode="_debug_direct").count().run(conn),
        r.db(dbName).table("uuid_pkey", read_mode="_debug_direct").count().run(conn2)]
    pprint.pprint(direct_counts)
    for num in direct_counts:
        assert 250 < num < 750

    utils.print_with_time("Testing sharding of existing inserted data")
    res = r.db(dbName).table_create("numeric_pkey").run(conn)
    assert res["tables_created"] == 1
    res = r.db(dbName).table("numeric_pkey").insert([{"id": n} for n in xrange(1000)]).run(conn)
    assert res["inserted"] == 1000 and res["errors"] == 0
    r.db(dbName).table("numeric_pkey").reconfigure(shards=10, replicas=1).run(conn)
    r.db(dbName).table("numeric_pkey").wait(wait_for="all_replicas_ready").run(conn)
    res = r.db(dbName).table("numeric_pkey").info().run(conn)["doc_count_estimates"]
    pprint.pprint(res)
    for num in res:
        assert 50 < num < 200

    utils.print_with_time("Creating an unbalanced table")
    res = r.db(dbName).table_create("unbalanced").run(conn)
    assert res["tables_created"] == 1
    r.db(dbName).table("unbalanced").reconfigure(shards=2, replicas=1).run(conn)
    r.db(dbName).table("unbalanced").wait(wait_for="all_replicas_ready").run(conn)
    res = r.db(dbName).table("unbalanced").insert([{"id": n} for n in xrange(1000)]).run(conn)
    assert res["inserted"] == 1000 and res["errors"] == 0
    res = r.db(dbName).table("unbalanced").info().run(conn)["doc_count_estimates"]
    pprint.pprint(res)
    assert res[0] > 500
    assert res[1] < 100

    # If we ever implement #2896, we should make sure the server has an issue now

    utils.print_with_time("Fixing the unbalanced table")
    status_before = r.db(dbName).table("unbalanced").status().run(conn)
    res = r.db(dbName).table("unbalanced").rebalance().run(conn)
    assert res["rebalanced"] == 1
    assert len(res["status_changes"]) == 1
    assert res["status_changes"][0]["old_val"] == status_before
    assert res["status_changes"][0]["new_val"]["status"]["all_replicas_ready"] == False
    r.db(dbName).table("unbalanced").wait(wait_for="all_replicas_ready").run(conn)
    res = r.db(dbName).table("unbalanced").info().run(conn)["doc_count_estimates"]
    pprint.pprint(res)
    for num in res:
        assert 250 < num < 750

    utils.print_with_time("Cleaning up")
utils.print_with_time("Done.")

