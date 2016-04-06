#!/usr/bin/env python
# Copyright 2010-2016 RethinkDB, all rights reserved.

import os, pprint, sys, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, rdb_unittest, scenario_common, utils, vcoptparse

opts = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(opts)
_, command_prefix, server_options = scenario_common.parse_mode_flags(opts.parse(sys.argv))

class NameCollision(rdb_unittest.RdbTestCase):
    servers = 2
    destructiveTest = True
    
    def test_server_name_collision(self):
        '''run two servers with the same name, then resolve the issue'''
        
        alpha = self.cluster[0]
        serverName = alpha.name
        
        # -- stop the first server
        alpha.stop()
        
        # -- startup another server with the same name
        beta = driver.Process(cluster=self.cluster, name=serverName, command_prefix=command_prefix, extra_options=server_options, console_output=True)
        
        # -- restart the first server
        alpha.start()
        
        # -- check for the server name collision
        issues = list(self.r.db("rethinkdb").table("current_issues").filter(self.r.row["type"] != "memory_error").run(self.conn))
        assert len(issues) == 1, pprint.pformat(issues)
        server_issue = issues[0]
        
        assert server_issue["type"] == "server_name_collision", server_issue
        assert server_issue["critical"], server_issue
        assert server_issue["info"]["name"] == serverName, server_issue
        assert set(server_issue["info"]["ids"]) == set([alpha.uuid, beta.uuid]), server_issue
        
        # -- resolve the server name collision
        res = self.r.db("rethinkdb").table("server_config").get(beta.uuid).update({"name":serverName + '2'}).run(self.conn)
        assert res["replaced"] == 1 and res["errors"] == 0, res
        
        # -- confirm the cluster is ok
        self.cluster.check()
    
    def test_db_name_collision(self):
        '''alternately shut down servers to create conflicting database names on them'''
        
        alpha = self.cluster[0]
        beta = self.cluster[1]
        
        db_name = 'db_name_collision'
        
        alpha_db_uuid = None
        beta_db_uuid = None
        
        # -- shut down the second server and setup the first
        beta.stop()
        
        alpha_db_uuid = self.r.db_create(db_name).run(self.conn)["config_changes"][0]["new_val"]["id"]
        deadline = time.time() + 5
        while time.time() < deadline:
            res = self.r.db_list().run(self.conn)
            if db_name in res:
                break
        else:
            raise AssertionError('Database name is not in db_list: %s' % db_name)
        
        # -- reverse and setup the second server
        alpha.stop()
        beta.start()
        beta_db_uuid = self.r.db_create(db_name).run(self.conn)["config_changes"][0]["new_val"]["id"]
        deadline = time.time() + 5
        while time.time() < deadline:
            res = self.r.db_list().run(self.conn)
            if db_name in res:
                break
        else:
            raise AssertionError('Database name is not in db_list: %s' % db_name)
        
        # -- bring up both servers and observe the error
        alpha.start()
        issues = list(self.r.db("rethinkdb").table("current_issues").filter(self.r.row["type"] != "memory_error").run(self.conn))
        assert len(issues) == 1, pprint.pformat(issues)
        db_issue = issues[0]
        
        assert db_issue["type"] == "db_name_collision", db_issue
        assert db_issue["critical"], db_issue
        assert db_issue["info"]["name"] == db_name, db_issue
        assert set(db_issue["info"]["ids"]) == set([alpha_db_uuid, beta_db_uuid]), db_issue
        
        # -- resolve the name issue
        res = self.r.db("rethinkdb").table("db_config").get(beta_db_uuid).update({"name":db_name + "2"}).run(self.conn)
        assert res["replaced"] == 1 and res["errors"] == 0, res
        
        # -- confirm the cluster is ok
        self.cluster.check()
    
    def test_table_name_collision(self):
        '''alternately shut down servers to create conflicting table names on them'''
        
        alpha = self.cluster[0]
        beta = self.cluster[1]
        
        table_name = 'table_name_collision'
        
        alpha_table_uuid = None
        beta_table_uuid = None
        
        # -- shut down the second server and setup the first
        beta.stop()
        
        alpha_table_uuid = self.db.table_create(table_name).run(self.conn)["config_changes"][0]["new_val"]["id"]
        self.db.table(table_name).wait(wait_for='all_replicas_ready').run(self.conn)
        
        # -- reverse and setup the second server
        alpha.stop()
        beta.start()
        
        beta_table_uuid = self.db.table_create(table_name).run(self.conn)["config_changes"][0]["new_val"]["id"]
        self.db.table(table_name).wait(wait_for='all_replicas_ready').run(self.conn)
        
        # -- bring up both servers and observe the error
        alpha.start()
        issues = list(self.r.db("rethinkdb").table("current_issues").filter(self.r.row["type"] != "memory_error").run(self.conn))
        assert len(issues) == 1, pprint.pformat(issues)
        table_issue = issues[0]

        assert table_issue["type"] == table_name, table_issue
        assert table_issue["critical"], table_issue
        assert table_issue["info"]["name"] == table_name, table_issue
        assert table_issue["info"]["db"] == self.dbName, table_issue
        assert set(table_issue["info"]["ids"]) == set([alpha_table_uuid, beta_table_uuid]), table_issue
        
        # -- resolve the name issue
        res = self.r.db("rethinkdb").table("table_config").get(beta_table_uuid).update({"name":table_name + "2"}).run(self.conn)
        assert res["replaced"] == 1 and res["errors"] == 0
        
        # -- confirm the cluster is ok
        self.cluster.check()

# ==== main

if __name__ == '__main__':
    rdb_unittest.main()
