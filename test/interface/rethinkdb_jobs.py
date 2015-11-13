#!/usr/bin/env python
# Copyright 2010-2015 RethinkDB, all rights reserved.

import os, socket, sys, threading, time

sys.path.append(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common'))
import rdb_unittest, utils

class JobsTableTests(rdb_unittest.RdbTestCase):
    '''Tests the artificial `rethinkdb.jobs` table'''

    servers = 2
    timeout = 20

    def test_query(self):
        taskLength = .5

        server = self.cluster[0]
        targetConn = self.r.connect(host=server.host, port=server.driver_port)
        host, port, _, _ = targetConn._instance._socket._socket.getsockname()

        filterObject = {"type":"query", "info":{"client_address":host, "client_port":port}}
        query = self.r.db("rethinkdb").table("jobs").filter(filterObject).coerce_to("ARRAY")

        # - start the target query

        self.r.js("while(true) {}", timeout=taskLength).run(targetConn, noreply=True)

        # - watch for the query in the rethinkdb.jobs table

        latestError = None
        deadline = time.time() + self.timeout
        while time.time() < deadline:
            try:
                response = query.run(self.conn)

                # Note the "info" and "type" fields are implicitly checked by the filter
                self.assertEqual(len(response), 1, 'Looking for 1 query, got: %d' % len(response))
                self.assertTrue(0 < response[0]["duration_sec"] <= taskLength)
                self.assertEqual(response[0]["id"][0], "query")
                self.assertEqual(response[0]["servers"], [server.name])

                break # break if successful, leaving query running
            except Exception as e:
                latestError = e
                time.sleep(.01)
        else:
            self.fail('Timed out after %.1f seconds waiting for query to appear. Last error was: %s' % (self.timeout, latestError.message))

    def test_job_deletion(self):
        (conn_a, conn_b) = [self.r.connect(host=s.host, port=s.driver_port) for s in self.cluster]
        result = None

        def query_thread(result):
            try:
                self.r.js("while(true) {}", timeout=10).run(conn_a)
                err = RuntimeError("Query should have been interrupted, but returned successfully.")
            except:
                (_, err, _) = sys.exc_info()
            result.append(err)

        result = list() # Use a list as an output parameter
        worker = threading.Thread(target=query_thread, args=[result])
        worker.start()

        deadline = time.time() + self.timeout
        job_filter = self.r.db('rethinkdb').table('jobs').filter(lambda x: x['info']['query'].match('^r\.js'))
        while len(list(job_filter.run(conn_b))) != 1 and time.time() < deadline:
            pass

        self.assertEqual(job_filter.delete()['deleted'].run(conn_b), 1)
        worker.join()
        self.assertEqual(len(result), 1)
        expected = "Query terminated by the `rethinkdb.jobs` table."
        if not isinstance(result[0], self.r.ReqlRuntimeError) or result[0].message != expected:
            self.fail('Unexpected error: %s' % str(result[0]))

    def test_disk_compaction(self):

        # - insert a record to update

        response = self.table.insert({}).run(self.conn)
        self.assertTrue('generated_keys' in response and isinstance(response['generated_keys'], list), response)
        sampleId = response['generated_keys'][0]

        writeQuery = self.r.range(100).for_each(lambda x: self.table.get(sampleId).update({'value':x}))
        readQuery = self.r.db("rethinkdb").table("jobs").filter({"type":"disk_compaction"}).coerce_to("ARRAY")

        deadline = time.time() + self.timeout
        latestError = None
        while time.time () < deadline:
            try:
                writeQuery.run(self.conn, noreply=True) # a reasonably fast way of triggering compaction

                response = readQuery.run(self.conn)

                # Note the "type" field is implicitly checked by the filter
                self.assertEqual(len(response), 1, 'Looking for 1 query, got: %d' % len(response))
                self.assertEqual(response[0]["duration_sec"], None)
                self.assertEqual(response[0]["id"][0], "disk_compaction")
                self.assertEqual(response[0]["servers"], [self.cluster[0].name])

                break # break if sucessfull, leaving query running
            except Exception as e:
                latestError = e
                time.sleep(.01)
        else:
            self.fail('Timed out after %.1f seconds waiting for disk_compaction to appear. Last error was: %s' % (self.timeout, latestError.message))

    def test_index_construction(self):

        valueDropsLimit = 10

        recordsToGenerate = 500
        fieldName = 'x'

        filterObject = {"type":"index_construction", "info":{"db":self.dbName, "table":self.tableName, "index":fieldName}}
        query = self.r.db("rethinkdb").table("jobs").filter(filterObject).coerce_to("ARRAY")

        # - generate some records

        utils.populateTable(conn=self.conn, table=self.table, records=recordsToGenerate, fieldName=fieldName)

        # - create an index

        self.table.index_create(fieldName).run(self.conn)

        # - watch the index_construction entry progress and then go away

        latestValue = None
        valueIncreases = 0

        deadline = time.time() + self.timeout
        valueDropMessages = []

        while time.time () < deadline:
            response = query.run(self.conn)

            self.assertTrue(len(response) in (0, 1), 'Expected only one filtered job, got: %r' % response)
            if len(response) == 1:

                # - check content

                try:
                    self.assertTrue('id' in response[0], response[0])
                    self.assertEqual(response[0]["id"][0], "index_construction")

                    self.assertTrue('servers' in response[0], response[0])
                    self.assertEqual(len(response[0]["servers"]), 1)

                    self.assertTrue('info' in response[0] and isinstance(response[0]['info'], dict), response[0])
                    self.assertTrue('db' in response[0]['info'] and response[0]['info']['db'] == self.dbName, response[0])
                    self.assertTrue('table' in response[0]['info'] and response[0]['info']['table'] == self.tableName, response[0])
                    self.assertTrue('index' in response[0]['info'] and response[0]['info']['index'] == fieldName, response[0])
                    self.assertTrue('progress' in response[0]['info'] and response[0]['info']['progress'] >= 0, response[0])

                    self.assertTrue('duration_sec' in response[0] and 0 < response[0]["duration_sec"] <= self.timeout, response[0])
                except Exception: pass

                # - ensure we are making progress

                if latestValue is None or response[0]["info"]["progress"] > latestValue:
                    latestValue = response[0]["info"]["progress"]
                    valueIncreases += 1
                elif response[0]["info"]["progress"] < latestValue:
                    valueDropsLimit -= 1
                    valueDropMessages.append('from %f to %f' % (latestValue, response[0]["info"]["progress"]))
                    latestValue = response[0]["info"]["progress"]
                    if valueDropsLimit < 1:
                        self.fail('progress value dropped too many times: %s' % (', '.join(valueDropMessages)))

            elif latestValue is not None:
                # we have seen an index_construction, and now it is gone
                if valueIncreases < 2:
                    self.fail('Did not see at least 2 value increases before index_construction entry dissapeared')
                break

            time.sleep(.01)
        else:
            if latestValue is None:
                self.fail('Timed out after %.1f seconds waiting for index_construction appear' % self.timeout)
            else:
                self.fail('Timed out after %.1f seconds waiting for index_construction to be complete' % self.timeout)

    def test_backfill(self):

        valueDropsLimit = 10

        recordsToGenerate = 10000
        fieldName = 'x'

        # - generate some records

        utils.populateTable(conn=self.conn, table=self.table, records=recordsToGenerate, fieldName=fieldName)

        # - trigger a backfill with a reconfigure

        reconfigureResponse = self.table.reconfigure(shards=1, replicas=2).run(self.conn)

        primaryReplica = reconfigureResponse["config_changes"][0]["new_val"]["shards"][0]["primary_replica"]
        seondaryReplicas = set(reconfigureResponse["config_changes"][0]["new_val"]["shards"][0]["replicas"]) - set(primaryReplica)

        # - watch for the entry in rethinkdb.jobs

        query = self.r.db("rethinkdb").table("jobs").filter({"type": "backfill"}).coerce_to("ARRAY")

        latestValue = None
        valueIncreases = 0

        deadline = time.time() + self.timeout
        valueDropMessages = []

        while time.time () < deadline:
            response = query.run(self.conn)

            self.assertTrue(len(response) in (0, 1), 'Expected only one backfill job, got: %r' % response)
            if len(response) == 1:

                # - check content

                try:
                    self.assertTrue('id' in response[0], response[0])
                    self.assertEqual(response[0]["id"][0], "backfill")

                    self.assertTrue('servers' in response[0], response[0])
                    self.assertEqual(len(response[0]["servers"]), self.servers)

                    self.assertTrue('info' in response[0] and isinstance(response[0]['info'], dict), response[0])
                    self.assertTrue('db' in response[0]['info'] and response[0]['info']['db'] == self.dbName, response[0])
                    self.assertTrue('table' in response[0]['info'] and response[0]['info']['table'] == self.tableName, response[0])
                    self.assertTrue('progress' in response[0]['info'] and response[0]['info']['progress'] >= 0, response[0])

                    self.assertTrue('source_server' in response[0]["info"] and response[0]["info"]['source_server'] == primaryReplica)
                    self.assertTrue('destination_server' in response[0]["info"] and response[0]["info"]['destination_server'] in seondaryReplicas)

                    self.assertTrue('duration_sec' in response[0] and 0 < response[0]["duration_sec"] <= self.timeout, response[0])
                except Exception: pass

                # - ensure we are making progress

                if latestValue is None or response[0]["info"]["progress"] > latestValue:
                    latestValue = response[0]["info"]["progress"]
                    valueIncreases += 1
                elif response[0]["info"]["progress"] < latestValue:
                    valueDropsLimit -= 1
                    valueDropMessages.append('from %f to %f' % (latestValue, response[0]["info"]["progress"]))
                    latestValue = response[0]["info"]["progress"]
                    if valueDropsLimit < 1:
                        self.fail('progress value dropped too many times: %s' % (', '.join(valueDropMessages)))

            elif latestValue is not None:
                # we have seen an index_construction, and now it is gone
                if valueIncreases < 2:
                    self.fail('Did not see at least 2 value increases before backfill entry dissapeared')
                break

            time.sleep(.01)
        else:
            if latestValue is None:
                self.fail('Timed out after %.1f seconds waiting for backfill appear' % self.timeout)
            else:
                self.fail('Timed out after %.1f seconds waiting for backfill to be complete' % self.timeout)

# ===== main

if __name__ == '__main__':
    rdb_unittest.main()
