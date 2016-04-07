#!/usr/bin/env python
# Copyright 2010-2016 RethinkDB, all rights reserved.

"""This test repeatedly reconfigures a table in a specific pattern to test the
efficiency of the reconfiguration logic. It's sort of like `shard_fuzzer.py` but it
produces a specific workload instead of a random one."""

from __future__ import print_function

import os, string, sys, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, rdb_unittest, scenario_common, utils, vcoptparse

try:
    xrange
except NameError:
    xrange = range

opts = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(opts)
opts['num-servers'] = vcoptparse.IntFlag('--num-servers', 2)
opts['num-tables'] = vcoptparse.IntFlag('--num-tables', 1)
opts['num-rows'] = vcoptparse.IntFlag('--num-rows', 10)
opts['num-shards'] = vcoptparse.IntFlag('--num-shards', 32)
opts['num-replicas'] = vcoptparse.IntFlag('--num-replicas', 1)
opts['num-phases'] = vcoptparse.IntFlag('--num-phases', 2)
parsed_opts = opts.parse(sys.argv)
_, command_prefix, server_options = scenario_common.parse_mode_flags(parsed_opts)

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

class ReconfigureStress(rdb_unittest.RdbTestCase):
    servers = server_names
    server_command_prefix = command_prefix
    server_extra_options = server_options
    
    def test_reconfigure_stress(self):
        print("") # to move us off the unittest line
        
        # Existing tables
        existingTables = self.db.table_list().run(self.conn)
        
        # Set up tables
        utils.print_with_time("\tSetting up %d table%s" % (len(table_names), "(s)" if len(table_names) > 1 else ""))
        for name in table_names:
            res = self.r.db("rethinkdb").table("table_config").insert({
                "name": name,
                "db": self.dbName,
                "shards": [{"primary_replica": "a", "replicas": ["a"]}]
                }).run(self.conn)
        res = self.db.wait(wait_for="all_replicas_ready").run(self.conn)
        self.assert_("ready" in res, "Key 'ready' not in result: %s" % res)
        self.assertEqual(res["ready"], len(table_names) + len(existingTables))
        res = self.db.table_list().run(self.conn)
        for tableName in table_names:
            self.assert_(tableName in table_names, 'Table "%s" is missing' % tableName)
        
        # Run the phases
        for phase in xrange(parsed_opts['num-phases']):
            utils.print_with_time("\tBeginning reconfiguration phase %d" % (phase + 1))
            shards = make_config_shards(phase)
            for name in table_names:
                res = self.db.table(name).config().update({"shards": shards}).run(self.conn)
                assert res["replaced"] == 1 or res["unchanged"] == 1, res
            utils.print_with_time("\tWaiting for table(s) to become ready")
            for name in table_names:
                res = self.db.table(name).wait(wait_for="all_replicas_ready", timeout=600).run(self.conn)
                assert res["ready"] == 1, res
                for config_shard, status_shard in zip(shards, self.db.table(name).status()["shards"].run(self.conn)):
                    # make sure issue #4265 didn't happen
                    assert status_shard["primary_replicas"] == [config_shard["primary_replica"]]

rdb_unittest.main()
