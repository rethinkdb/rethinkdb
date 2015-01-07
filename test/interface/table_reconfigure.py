#!/usr/bin/env python
# Copyright 2014 RethinkDB, all rights reserved.

"""The `interface.table_reconfigure` test checks that the `table.reconfigure()` method works as expected."""

from __future__ import print_function

import sys, os, time

try:
    xrange
except NameError:
    xrange = range

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

r = utils.import_python_driver()
dbName, tableName = utils.get_test_db_table()

num_servers = 5

print("Starting cluster of %d servers (%.2fs)" % (num_servers, time.time() - startTime))
with driver.Cluster(output_folder='.') as cluster:
    
    for i in xrange(1, num_servers+1):
        driver.Process(cluster=cluster, files="s%d" % i, server_tags=["tag_%d" % i], command_prefix=command_prefix, extra_options=serve_options)
    cluster.wait_until_ready()
    cluster.check()
    
    tag_table = {"default": ["s%d" % (i+1) for i in xrange(num_servers)]}
    for i in xrange(num_servers):
        tag_table["tag_%d" % (i+1)] = ["s%d" % (i+1)]
    
    server_names = [server.name for server in cluster]
    
    print("Establishing ReQl connections (%.2fs)" % (time.time() - startTime))
    
    conn = r.connect(host=cluster[0].host, port=cluster[0].driver_port)
    
    print("Creating a table (%.2fs)" % (time.time() - startTime))
    
    if dbName not in r.db_list().run(conn):
        r.db_create(dbName).run(conn)
    if tableName in r.db(dbName).table_list().run(conn):
        r.db(dbName).table_drop(tableName).run(conn)
    r.db(dbName).table_create(tableName).run(conn)
    res = r.db(dbName).table(tableName).config() \
        .update({"shards": [{"primary_replica": "s1", "replicas": ["s1"]}]}).run(conn)
    assert res["errors"] == 0
    r.db(dbName).table(tableName).wait().run(conn)
    
    print("Adding data (%.2fs)" % (time.time() - startTime))
    
    # Insert some data so distribution queries can work
    r.db(dbName).table(tableName).insert([{"x":x} for x in xrange(100)]).run(conn)
    
    print("Test reconfigure dry_run (%.2fs)" % (time.time() - startTime))
    
    # Generate many configurations using `dry_run=True` and check to make sure they
    # satisfy the constraints
    def test_reconfigure(num_shards, num_replicas, primary_replica_tag):
        
        print("Making configuration num_shards=%d num_replicas=%r primary_replica_tag=%r (%.2fs)" % (num_shards, num_replicas, primary_replica_tag, time.time() - startTime))
        res = r.db(dbName).table(tableName).reconfigure(shards=num_shards,
            replicas=num_replicas, primary_replica_tag=primary_replica_tag, dry_run=True).run(conn)
        assert res["reconfigured"] == 0
        assert len(res["config_changes"]) == 1
        assert res["config_changes"][0]["old_val"] == \
            r.db(dbName).table(tableName).config().run(conn)
        new_config = res["config_changes"][0]['new_val']
        print(new_config)

        # Make sure new config follows all the rules
        assert len(new_config["shards"]) == num_shards
        for shard in new_config["shards"]:
            assert shard["primary_replica"] in tag_table[primary_replica_tag]
            for tag, count in num_replicas.iteritems():
                servers_in_tag = [s for s in shard["replicas"] if s in tag_table[tag]]
                assert len(servers_in_tag) == count
            assert len(shard["replicas"]) == sum(num_replicas.values())
        primary_replicas = set(shard["primary_replica"] for shard in new_config["shards"])

        # Make sure new config distributes replicas evenly when possible
        assert len(primary_replicas) == min(num_shards, num_servers)
        for tag, count in num_replicas.iteritems():
            usages = {}
            for shard in new_config["shards"]:
                for replica in shard["replicas"]:
                    usages[replica] = usages.get(replica, 0) + 1
            if max(usages.values()) > min(usages.values()) + 1:
                # The current algorithm will sometimes fail to distribute replicas
                # evenly. See issue #3028. Since this is a known issue, we just print a
                # warning instead of failing the test.
                print("WARNING: unevenly distributed replicas:", usages)

        return new_config

    for num_shards in [1, 2, num_servers-1, num_servers, num_servers+1, num_servers*2]:
        for num_replicas in [1, 2, num_servers-1, num_servers]:
            test_reconfigure(num_shards, {"default": num_replicas}, "default")
    test_reconfigure(1, {"tag_1": 1, "tag_2": 1}, "tag_1")
    test_reconfigure(1, {"tag_1": 1, "tag_2": 1}, "tag_2")
    test_reconfigure(1, {"tag_1": 1}, "tag_1")
    
    print("Test table_create dry_run (%.2fs)" % (time.time() - startTime))
    
    # Test to make sure that `dry_run` is respected; the config should only be stored in
    # the semilattices if `dry_run` is `False`.
    status_before = r.db(dbName).table(tableName).status().run(conn)
    config_before = r.db(dbName).table(tableName).config().run(conn)
    dry_run_res = r.db(dbName).table(tableName).reconfigure(
        shards=1, replicas={"tag_2": 1}, primary_replica_tag="tag_2", dry_run=True).run(conn)
    status_between = r.db(dbName).table(tableName).status().run(conn)
    config_between = r.db(dbName).table(tableName).config().run(conn)
    wet_run_res = r.db(dbName).table(tableName).reconfigure(
        shards=1, replicas={"tag_2": 1}, primary_replica_tag="tag_2", dry_run=False).run(conn)
    config_after = r.db(dbName).table(tableName).config().run(conn)
    assert dry_run_res["reconfigured"] == 0, dry_run_res
    assert wet_run_res["reconfigured"] == 1, wet_run_res
    assert dry_run_res["config_changes"][0]["old_val"] == config_before
    assert dry_run_res["config_changes"][0]["new_val"] != config_before
    assert config_before == config_between
    assert wet_run_res["config_changes"][0]["old_val"] == config_between
    assert wet_run_res["config_changes"][0]["new_val"] == config_after
    assert config_after != config_between
    assert "status_changes" not in dry_run_res
    assert status_before == status_between
    assert wet_run_res["status_changes"][0]["old_val"] == status_between
    assert wet_run_res["status_changes"][0]["new_val"] != status_between
    
    print("Test table_create parameters (%.2fs)" % (time.time() - startTime))
    
    # Test that configuration parameters to `table_create()` work
    res = r.db(dbName).table_create("blablabla", replicas=2, shards=8).run(conn)
    assert res["tables_created"] == 1
    conf = r.db(dbName).table("blablabla").config().run(conn)
    assert len(conf["shards"]) == 8
    for i in xrange(8):
        assert len(conf["shards"][i]["replicas"]) == 2
    res = r.db(dbName).table_drop("blablabla").run(conn)
    assert res["tables_dropped"] == 1
    
    print("Test table re-creation preference (%.2fs)" % (time.time() - startTime))
    
    # Test that we prefer servers that held our data before
    for server in server_names:
        res = r.db(dbName).table(tableName).config() \
            .update({"shards": [{"replicas": [server], "primary_replica": server}]}).run(conn)
        assert res["errors"] == 0, repr(res)
        for i in xrange(10):
            time.sleep(3)
            if r.db(dbName).table(tableName).status().run(conn)["status"]["all_replicas_ready"]:
                break
        else:
            raise Exception("took too long to reconfigure")
        new_config = test_reconfigure(2, {"default": 1}, "default")
        if (new_config["shards"][0]["primary_replica"] != server and
                new_config["shards"][1]["primary_replica"] != server):
            raise Exception("expected to prefer %r, instead got %r" % (server, new_config))

    res = r.db(dbName).table_drop(tableName).run(conn)
    assert res["tables_dropped"] == 1
    
    print("Test table provisioning preference (%.2fs)" % (time.time() - startTime))
    
    # Test that we prefer servers that aren't holding other tables' data. We do this by
    # constructing a table "blocker", which we configure so it uses all but one server;
    # then we create a table "probe", and assert that it resides on the one unused
    # server.
    res = r.db(dbName).table_create("blocker").run(conn)
    assert res["tables_created"] == 1
    for server in server_names:
        res = r.db(dbName).table("blocker").config().update({"shards": [{
            "replicas": [n for n in server_names if n != server],
            "primary_replica": [n for n in server_names if n != server][0]
            }]}).run(conn)
        assert res["errors"] == 0
        for i in xrange(10):
            time.sleep(3)
            if r.db(dbName).table("blocker").status() \
                    .run(conn)["status"]["all_replicas_ready"]:
                break
        else:
            raise ValueError("took too long to reconfigure")
        res = r.db(dbName).table_create("probe").run(conn)
        assert res["tables_created"] == 1
        probe_config = r.db(dbName).table("probe").config().run(conn)
        assert probe_config["shards"][0]["replicas"] == [server]
        res = r.db(dbName).table_drop("probe").run(conn)
        assert res["tables_dropped"] == 1

    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))

