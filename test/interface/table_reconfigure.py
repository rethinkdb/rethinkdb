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
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)

    num_servers = 5
    print "Spinning up %d processes..." % num_servers
    files = [driver.Files(metacluster,
                          log_path = "create-output-%d" % (i+1),
                          machine_name = "server_%d" % (i+1),
                          server_tags = ["tag_%d" % (i+1)],
                          executable_path = executable_path,
                          command_prefix = command_prefix)
        for i in xrange(num_servers)]
    procs = [driver.Process(cluster,
                            files[i],
                            log_path = "serve-output-%d" % (i+1),
                            executable_path = executable_path,
                            command_prefix = command_prefix,
                            extra_options = serve_options)
        for i in xrange(num_servers)]
    for p in procs:
        p.wait_until_started_up()
    cluster.check()

    tag_table = {"default": ["server_%d" % (i+1) for i in xrange(num_servers)]}
    for i in xrange(num_servers):
        tag_table["tag_%d" % (i+1)] = ["server_%d" % (i+1)]

    print "Creating a table..."
    conn = r.connect("localhost", procs[0].driver_port)
    r.db_create("test").run(conn)
    r.table_create("foo").run(conn)

    # Insert some data so distribution queries can work
    r.table("foo").insert([{"x":x} for x in xrange(100)]).run(conn)

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
    for num_shards in [1, 2, num_servers-1, num_servers, num_servers+1, num_servers*2]:
        for num_replicas in [1, 2, num_servers-1, num_servers]:
            test_reconfigure(num_shards, {"default": num_replicas}, "default")
    test_reconfigure(1, {"tag_1": 1, "tag_2": 1}, "tag_1")
    test_reconfigure(1, {"tag_1": 1, "tag_2": 1}, "tag_2")
    test_reconfigure(1, {"tag_1": 1}, "tag_1")

    # Test to make sure that `dry_run` is respected; the config should only be stored in
    # the semilattices if `dry_run` is `False`.
    def get_config():
        row = r.table_config('foo').run(conn)
        del row["name"]
        del row["db"]
        del row["uuid"]
        return row
    prev_config = get_config()
    new_config = r.table("foo").reconfigure(1, {"tag_2": 1}, director_tag="tag_2",
        dry_run=True).run(conn)
    assert prev_config != new_config
    assert get_config() == prev_config
    new_config_2 = r.table("foo").reconfigure(1, {"tag_2": 1}, director_tag="tag_2",
        dry_run=False).run(conn)
    assert prev_config != new_config_2
    print "get_config()", get_config()
    print "new_config_2", new_config_2
    print "prev_config", prev_config
    assert get_config() == new_config_2

    cluster.check_and_stop()
print "Done."

