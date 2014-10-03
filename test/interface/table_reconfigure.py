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
                          machine_name = "s%d" % (i+1),
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
    server_names = ["s%d" % (i+1) for i in xrange(num_servers)]
    for p in procs:
        p.wait_until_started_up()
    cluster.check()

    tag_table = {"default": ["s%d" % (i+1) for i in xrange(num_servers)]}
    for i in xrange(num_servers):
        tag_table["tag_%d" % (i+1)] = ["s%d" % (i+1)]

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

        # Make sure new config follows all the rules
        assert len(new_config["shards"]) == num_shards
        for shard in new_config["shards"]:
            assert shard["director"] in tag_table[director_tag]
            for tag, count in num_replicas.iteritems():
                servers_in_tag = [s for s in shard["replicas"] if s in tag_table[tag]]
                assert len(servers_in_tag) == count
            assert len(shard["replicas"]) == sum(num_replicas.values())
        directors = set(shard["director"] for shard in new_config["shards"])

        # Make sure new config distributes replicas evenly when possible
        assert len(directors) == min(num_shards, num_servers)
        for tag, count in num_replicas.iteritems():
            usages = {}
            for shard in new_config["shards"]:
                for replica in shard["replicas"]:
                    usages[replica] = usages.get(replica, 0) + 1
            if max(usages.values()) > min(usages.values()) + 1:
                # The current algorithm will sometimes fail to distribute replicas
                # evenly. See issue #3028. Since this is a known issue, we just print a
                # warning instead of failing the test.
                print "WARNING: unevenly distributed replicas:", usages

        return new_config

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
        del row["primary_key"]
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

    # Test that we prefer servers that held our data before
    for server in server_names:
        res = r.table_config("foo") \
               .update({"shards": [{"replicas": [server], "director": server}]}) \
               .run(conn)
        assert res["errors"] == 0, repr(res)
        for i in xrange(10):
            time.sleep(3)
            if r.table_status("foo").run(conn)["ready_completely"]:
                break
        else:
            raise ValueError("took too long to reconfigure")
        new_config = test_reconfigure(2, {"default": 1}, "default")
        if (new_config["shards"][0]["director"] != server and
                new_config["shards"][1]["director"] != server):
            raise ValueError("expected to prefer %r, instead got %r" % \
                (server, new_config))

    res = r.table_drop("foo").run(conn)
    assert res == {"dropped": 1}

    # Test that we prefer servers that aren't holding other tables' data. We do this by
    # constructing a table "blocker", which we configure so it uses all but one server;
    # then we create a table "probe", and assert that it resides on the one unused
    # server.
    res = r.table_create("blocker").run(conn)
    assert res == {"created": 1}
    for server in server_names:
        res = r.table_config("blocker").update({"shards": [{
            "replicas": [n for n in server_names if n != server],
            "director": [n for n in server_names if n != server][0]
            }]}).run(conn)
        assert res["errors"] == 0
        for i in xrange(10):
            time.sleep(3)
            if r.table_status("blocker").run(conn)["ready_completely"]:
                break
        else:
            raise ValueError("took too long to reconfigure")
        res = r.table_create("probe").run(conn)
        assert res == {"created": 1}
        probe_config = r.table_config("probe").run(conn)
        assert probe_config["shards"][0]["replicas"] == [server]
        res = r.table_drop("probe").run(conn)
        assert res == {"dropped": 1}

    cluster.check_and_stop()
print "Done."

