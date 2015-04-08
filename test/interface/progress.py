#!/usr/bin/env python
# Copyright 2010-2015 RethinkDB, all rights reserved.

import os, socket, sys, threading, time

sys.path.append(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common'))
import rdb_unittest

class LongRunningQuery(threading.Thread):
    def __init__(self, conn, timeout=30):
        super(LongRunningQuery, self).__init__()

        self._conn = conn
        self._r = self._conn._r
        self._timeout = timeout

    def run(self):
        try:
            self._r.js("while(true) {}", timeout=self._timeout).run(self._conn)
        except self._r.RqlDriverError as error:
            if error.message != "Connection is closed.":
                raise

class JobsTestCase(rdb_unittest.RdbTestCase):
    '''Tests the artificial `rethinkdb.jobs` table'''

    servers = 2
    fieldName = 'x'
    recordsToGenerate = 10000

    destructiveTest = True

    def test_query(self):
        server = self.cluster[0]
        conn = self.r.connect(host=server.host, port=server.driver_port)

        query = LongRunningQuery(conn)
        query.daemon = True
        query.start()

        # The query timeout is 30 seconds by default, wait for `test_duration` to check
        # the `duration` field of the response.
        test_duration = 2
        time.sleep(test_duration)

        host, port, _, _ = conn._instance._socket._socket.getsockname()
        response = self.r.db("rethinkdb") \
                         .table("jobs") \
                         .filter({"type": "query",
                                  "info": {
                                      "client_address": host,
                                      "client_port": port}}) \
                         .coerce_to("ARRAY") \
                         .run(self.conn)

        # Note the "info" and "type" fields are implicitly checked by the filter
        self.assertEqual(len(response), 1)
        self.assertTrue(response[0]["duration_sec"] >= test_duration)   # assertGreaterEqual in 2.7
        self.assertEqual(response[0]["id"][0], "query")
        self.assertEqual(response[0]["servers"], [server.name])

    def test_index_construction_disk_compaction(self):
        self.r.db(self.dbName) \
              .table(self.tableName) \
              .index_create(self.fieldName) \
              .run(self.conn)

        response = self.r.db("rethinkdb") \
                         .table("jobs") \
                         .filter({"type": "index_construction",
                                  "info": {
                                      "db": self.dbName,
                                      "table": self.tableName,
                                      "index": self.fieldName}}) \
                         .coerce_to("ARRAY") \
                         .run(self.conn)

        # Note the "info" and "type" fields are implicitly checked by the filter
        self.assertEqual(len(response), 1)
        self.assertTrue(response[0]["duration_sec"] >= 0)   # assertGreaterEqual in 2.7
        self.assertEqual(response[0]["id"][0], "index_construction")
        self.assertTrue(0 <= response[0]["info"]["progress"] <= 1)
        self.assertEqual(len(response[0]["servers"]), 1)

        response_two = self.r.db("rethinkdb") \
                             .table("jobs") \
                             .filter({"type": "index_construction",
                                      "info": {
                                          "table": self.tableName,
                                          "index": self.fieldName}}) \
                             .coerce_to("ARRAY") \
                             .run(self.conn)

        self.assertEqual(len(response_two), 1)
        self.assertTrue(response[0]["duration_sec"] <= response_two[0]["duration_sec"])
        # Note, due to the way progress is calculate it may actually decrease marginally
        self.assertTrue(
            response[0]["info"]["progress"] <= (response_two[0]["info"]["progress"] + 0.01))

        self.r.db(self.dbName) \
              .table(self.tableName) \
              .index_wait(self.fieldName) \
              .run(self.conn)

        # -- dropping an index is the easiest way of triggering disk compaction

        self.r.db(self.dbName) \
              .table(self.tableName) \
              .index_drop(self.fieldName) \
              .run(self.conn)

        response_three = self.r.db("rethinkdb") \
                               .table("jobs") \
                               .filter({"type": "disk_compaction"}) \
                               .coerce_to("ARRAY") \
                               .run(self.conn)

        # Note the "type" field is implicitly checked by the filter
        self.assertEqual(len(response_three), 1)
        self.assertEqual(response_three[0]["duration_sec"], None)
        self.assertEqual(response_three[0]["id"][0], "disk_compaction")
        self.assertEqual(response_three[0]["servers"], response[0]["servers"])

    def test_backfill(self):
        reconfigure = self.r.db(self.dbName) \
                            .table(self.tableName) \
                            .reconfigure(shards=1, replicas=2) \
                            .run(self.conn)

        # The directory updates take some time to propegate
        time.sleep(0.5)

        response = self.r.db("rethinkdb") \
                         .table("jobs") \
                         .filter({"type": "backfill"}) \
                         .coerce_to("ARRAY") \
                         .run(self.conn)

        # Note the "type" field is implicitly checked by the filter
        self.assertEqual(len(response), 1)
        self.assertTrue(response[0]["duration_sec"] >= 0)
        self.assertEqual(response[0]["id"][0], "backfill")
        self.assertTrue(
            response[0]["info"]["destination_server"] in
                (set(reconfigure["config_changes"][0]["new_val"]["shards"][0]["replicas"]) -
                set((reconfigure["config_changes"][0]["new_val"]["shards"][0]["primary_replica"], ))))
        self.assertEqual(response[0]["info"]["table"], self.tableName)
        self.assertEqual(response[0]["info"]["db"], self.dbName)
        self.assertEqual(
            response[0]["info"]["source_server"],
            reconfigure["config_changes"][0]["new_val"]["shards"][0]["primary_replica"])
        self.assertTrue(0 <= response[0]["info"]["progress"] <= 1)
        self.assertEqual(len(response[0]["servers"]), 2)

        time.sleep(0.5)

        response_two = self.r.db("rethinkdb") \
                             .table("jobs") \
                             .filter({"type": "backfill"}) \
                             .coerce_to("ARRAY") \
                             .run(self.conn)

        self.assertEqual(len(response_two), 1)
        self.assertTrue(response[0]["duration_sec"] <= response_two[0]["duration_sec"])
        self.assertTrue(
            response[0]["info"]["progress"] <= response_two[0]["info"]["progress"])

if __name__ == '__main__':
    import unittest
    unittest.main()
