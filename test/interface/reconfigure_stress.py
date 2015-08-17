#!/usr/bin/env python
# Copyright 2010-2015 RethinkDB, all rights reserved.

"""This test repeatedly reconfigures a table in a specific pattern to test the
efficiency of the reconfiguration logic. It's sort of like `shard_fuzzer.py` but it
produces a specific workload instead of a random one."""

from __future__ import print_function

import os, pprint, random, string, sys, threading, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

opts = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(opts)
opts['num-servers'] = vcoptparse.IntFlag('--num-servers', 2)
opts['num-tables'] = vcoptparse.IntFlag('--num-tables', 1)
opts['num-rows'] = vcoptparse.IntFlag('--num-rows', 10)
opts['num-shards'] = vcoptparse.IntFlag('--num-shards', 32)
opts['num-replicas'] = vcoptparse.IntFlag('--num-replicas', 1)
opts['num-phases'] = vcoptparse.IntFlag('--num-phases', 2)
parsed_opts = opts.parse(sys.argv)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(parsed_opts)

possible_server_names = \
    list(string.ascii_lowercase) + \
    [x + y for x in string.ascii_lowercase for y in string.ascii_lowercase]
assert parsed_opts["num-servers"] <= len(possible_server_names)
server_names = possible_server_names[:parsed_opts["num-servers"]]

table_names = ["table%d" % i for i in xrange(parsed_opts["num-tables"])]

def make_config_shards(phase):
    shards = []
    for i in xrange(parsed_opts['num-shards']):
        shard = {}
        shard["primary_replica"] = server_names[(i + phase) % len(server_names)]
        shard["replicas"] = []
        assert parsed_opts["num-replicas"] <= len(server_names)
        for j in xrange(parsed_opts['num-replicas']):
            shard["replicas"].append(server_names[(i + j + phase) % len(server_names)])
        shards.append(shard)
    return shards

r = utils.import_python_driver()

print("Spinning up %d servers (%.2fs)" % (len(server_names), time.time() - startTime))
with driver.Cluster(initial_servers=server_names, output_folder='.', command_prefix=command_prefix,
                    extra_options=serve_options, wait_until_ready=True) as cluster:
    cluster.check()
    
    print("Establishing ReQL connection (%.2fs)" % (time.time() - startTime))
    conn = r.connect(host=cluster[0].host, port=cluster[0].driver_port)

    print("Setting up table(s) (%.2fs)" % (time.time() - startTime))
    res = r.db_create("test").run(conn)
    assert res["dbs_created"] == 1, res
    for name in table_names:
        res = r.db("rethinkdb").table("table_config").insert({
            "name": name,
            "db": "test",
            "shards": [{"primary_replica": "a", "replicas": ["a"]}]
            }).run(conn)
        assert res["inserted"] == 1, res
    for name in table_names:
        res = r.table(name).wait().run(conn)
        assert res["ready"] == 1, res
        res = r.table(name).insert(
            r.range(0, parsed_opts["num-rows"]).map({"x": r.row})).run(conn)
        assert res["inserted"] == parsed_opts["num-rows"], res

    for phase in xrange(parsed_opts['num-phases']):
        print("Beginning reconfiguration phase %d (%.2fs)" % (phase + 1, time.time() - startTime))
        shards = make_config_shards(phase)
        for name in table_names:
            res = r.table(name).config().update({"shards": shards}).run(conn)
            assert res["replaced"] == 1 or res["unchanged"] == 1, res
        print("Waiting for table(s) to become ready (%.2fs)" % (time.time() - startTime))
        for name in table_names:
            res = r.table(name).wait(wait_for = "all_replicas_ready", timeout = 600).run(conn)
            assert res["ready"] == 1, res
            for config_shard, status_shard in zip(shards, r.table(name).status()["shards"].run(conn)):
                # make sure issue #4265 didn't happen
                assert status_shard["primary_replicas"] == [config_shard["primary_replica"]]

    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))

