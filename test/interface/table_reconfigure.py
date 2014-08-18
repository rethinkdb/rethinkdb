#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.
import sys, os, time, traceback
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils
from vcoptparse import *
r = utils.import_python_driver()

"""The `interface.table_reconfigure` test checks that the `table.reconfigure()` method
works as expected."""

op = OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster1 = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    print "Spinning up two processes..."
    files1 = driver.Files(metacluster, log_path = "create-output-1",
                          machine_name = "a", server_tags = ["a_tag"],
                          executable_path = executable_path, command_prefix = command_prefix)
    proc1 = driver.Process(cluster1, files1, log_path = "serve-output-1",
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
    files2 = driver.Files(metacluster, log_path = "create-output-2",
                          machine_name = "b", server_tags = ["b_tag"],
                          executable_path = executable_path, command_prefix = command_prefix)
    proc2 = driver.Process(cluster1, files2, log_path = "serve-output-2",
        executable_path = executable_path, command_prefix = command_prefix, extra_options = serve_options)
    proc1.wait_until_started_up()
    proc2.wait_until_started_up()
    cluster1.check()
    tag_table = {"default": ["a","b"], "a_tag": ["a"], "b_tag": ["b"]}

    print "Creating a table..."
    conn = r.connect("localhost", proc1.driver_port)
    r.db_create("test").run(conn)
    r.table_create("foo").run(conn)

    # Generate many configurations using `dry_run=True` and check to make sure they
    # satisfy the constraints
    def test_reconfigure(num_shards, num_replicas, director_tag):
        print "Making configuration",
        print "num_shards=%d" % num_shards,
        print "num_replicas=%r" % num_replicas,
        print "director_tag=%r" % director_tag
        new_config = r.table("foo").reconfigure(
            num_shards,
            num_replicas,
            director_tag = director_tag,
            dry_run = True).run(conn)
        print new_config
        assert len(new_config["shards"]) == num_shards
        for shard in new_config["shards"]:
            for server in shard["directors"]:
                assert server in tag_table[director_tag]
            for tag, count in num_replicas.iteritems():
                servers_in_tag = [s for s in shard["replicas"] if s in tag_table[tag]]
                assert len(servers_in_tag) == count
            assert len(shard["replicas"]) == sum(num_replicas.values())
    test_reconfigure(1, {"default": 1}, "default")
    test_reconfigure(2, {"default": 1}, "default")
    test_reconfigure(1, {"default": 2}, "default")
    test_reconfigure(2, {"default": 2}, "default")
    test_reconfigure(1, {"a_tag": 1, "b_tag": 1}, "a_tag")
    test_reconfigure(1, {"a_tag": 1, "b_tag": 1}, "b_tag")
    test_reconfigure(1, {"a_tag": 1}, "a_tag")

    # Test to make sure that `dry_run` is respected; the config should only be stored in
    # the semilattices if `dry_run` is `False`.
    def get_config():
        row = r.db("rethinkdb").table("table_config").get("test.foo").run(conn)
        del row["name"]
        del row["uuid"]
        return row
    prev_config = get_config()
    new_config = r.table("foo").reconfigure(1, {"b_tag": 1}, director_tag="b_tag",
        dry_run=True).run(conn)
    assert prev_config != new_config
    assert get_config() == prev_config
    new_config_2 = r.table("foo").reconfigure(1, {"b_tag": 1}, director_tag="b_tag",
        dry_run=False).run(conn)
    assert prev_config != new_config_2
    print "get_config()", get_config()
    print "new_config_2", new_config_2
    print "prev_config", prev_config
    assert get_config() == new_config_2

    cluster1.check_and_stop()
print "Done."

