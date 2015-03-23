#!/usr/bin/env python
# Copyright 2014-2015 RethinkDB, all rights reserved.

import itertools, os, sys, time

sys.path.append(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common'))
import rdb_unittest, utils

# ---

class SquashBase(rdb_unittest.RdbTestCase):
    '''Squash tests'''

    recordsToGenerate = 0

    def setUp(self, squash, field, generator, records, limit, multi=False):
        super(SquashBase, self).setUp()

        self._squash = squash
        self._field = field
        self._generator = generator
        self._records = records
        self._limit = limit
        self._multi = multi

        self._generator_initial = []
        for x in xrange(20):
            self._generator_initial.append((x, next(self._generator)))

        if self._multi:
            self._multi_id_generator = itertools.count(20)
            self._multi_len = len(self._generator_initial[0][1])

        self._primary_key = self.r.db(self.dbName) \
                                  .table(self.tableName) \
                                  .info()["primary_key"] \
                                  .run(self.conn)

        for x in xrange(self._records):
            self.r.db(self.dbName) \
                  .table(self.tableName) \
                  .insert(self._document(next(self._generator))) \
                  .run(self.conn)

        if self._field != self._primary_key:
            self.r.db(self.dbName) \
                  .table(self.tableName) \
                  .index_create(self._field, multi=self._multi) \
                  .run(self.conn)

            self.r.db(self.dbName) \
                  .table(self.tableName) \
                  .index_wait(self._field) \
                  .run(self.conn)

        self._feed_conn = self.r.connect(
            self.cluster[0].host, self.cluster[0].driver_port) 

    def tearDown(self):
        self.r.db(self.dbName) \
              .table(self.tableName) \
              .filter({"insert": True}) \
              .delete() \
              .run(self.conn)

        super(SquashBase, self).tearDown()

    def _document(self, value, key=None, key_generate=True):
        document = {
            self._field: value,
            "insert": True
        }

        # An increasing primary key is automatically added to multi tests as they
        # may influence sorting.
        if key is None and key_generate and self._multi:
            key = "g-%i" % next(self._multi_id_generator)

        if key is not None:
            self.assertTrue(self._field != self._primary_key)
            document[self._primary_key] = key

        return document

    def test_insert(self):
        query = self.r.db(self.dbName) \
                      .table(self.tableName) \
                      .order_by(index=self.r.desc(self._field)) \
                      .limit(self._limit) \
                      .changes(squash=self._squash)

        with utils.NextWithTimeout(query.run(self._feed_conn), stopOnEmpty=False) as feed:
            changes = min(self._records, self._limit)
            if self._multi:
                changes = min(
                    self._records * self._multi_len, self._limit)

            initial = []
            for x in xrange(changes):
                initial.append(next(feed))

            if self._records >= self._limit:
                _, value = self._generator_initial.pop()
                self.r.db(self.dbName) \
                      .table(self.tableName) \
                      .insert(self._document(value)) \
                      .run(self.conn)
                
                self.assertRaises(Exception, feed.next)

            value = next(self._generator)
            document = self._document(value)
            key = self.r.db(self.dbName) \
                        .table(self.tableName) \
                        .insert(document, return_changes=True) \
                        .run(self.conn) \
                        .get("generated_keys", [document.get("id", value)])[0]

            changes = 1
            if self._multi:
                changes = min(self._multi_len, self._limit)

            for x in xrange(changes):
                feed_next = next(feed)

                self.assertTrue("old_val" in feed_next)
                self.assertTrue("new_val" in feed_next)
                if len(initial) + x >= self._limit:
                    self.assertEqual(
                        feed_next["old_val"][self._field],
                        initial[x]["new_val"][self._field])
                else:
                    self.assertEqual(feed_next["old_val"], None)
                self.assertEqual(feed_next["new_val"]["id"], key)
                self.assertEqual(feed_next["new_val"][self._field], value)

    def test_insert_batch(self):
        # FIXME: Python 2.7 has new facilities allowing tests to be skipped, use those
        # when we no longer need to support 2.6
        if self._squash == True:
            # With squash True it might not squash agressively enough for this to be
            # predictable, skip it
            return

        query = self.r.db(self.dbName) \
                      .table(self.tableName) \
                      .order_by(index=self.r.desc(self._field)) \
                      .limit(self._limit) \
                      .changes(squash=self._squash)

        with utils.NextWithTimeout(query.run(self._feed_conn), stopOnEmpty=False) as feed:
            changes = min(self._records, self._limit)
            if self._multi:
                changes = min(
                    self._records * self._multi_len, self._limit)

            initial = []
            for x in xrange(changes): 
                initial.append(next(feed))

            keys, documents = [], []
            for x in xrange(self._limit + 2):
                value = next(self._generator)
                if self._field == self._primary_key:
                    keys.append(value)
                    documents.append(self._document(value))
                else:
                    keys.append(x)
                    documents.append(self._document(value, x))

            # A document with duplicate primary key should be ignored
            error = documents[-1].copy()
            error.update({"error": True})
            documents.append(error)

            self.r.db(self.dbName) \
                  .table(self.tableName) \
                  .insert(documents) \
                  .run(self.conn)

            for x in xrange(self._limit):
                feed_next = next(feed)

                self.assertTrue("old_val" in feed_next)
                self.assertTrue("new_val" in feed_next)
                if len(initial) + x >= self._limit:
                    self.assertTrue(
                        feed_next["old_val"] in map(lambda x: x["new_val"], initial))
                else:
                    self.assertEqual(feed_next["old_val"], None)
                self.assertTrue(feed_next["new_val"] in documents[2:])
                self.assertTrue(not "error" in feed_next["new_val"])

        # self.assertRaises(Exception, feed.next)

    def test_delete(self):
        query = self.r.db(self.dbName) \
                      .table(self.tableName) \
                      .order_by(index=self.r.desc(self._field)) \
                      .limit(self._limit) \
                      .changes(squash=self._squash) 

        with utils.NextWithTimeout(query.run(self._feed_conn), stopOnEmpty=False) as feed:
            changes = min(self._records, self._limit)
            if self._multi:
                changes = min(
                    self._records * self._multi_len, self._limit)

            initial = []
            for x in xrange(changes): 
                initial.append(next(feed))

            # self.assertRaises(Exception, feed.next)

            if self._records >= self._limit:
                _, value = self._generator_initial.pop()
                key = self.r.db(self.dbName) \
                            .table(self.tableName) \
                            .insert(self._document(value), return_changes=True) \
                            .run(self.conn).get("generated_keys", [value])[0]

                self.r.db(self.dbName) \
                      .table(self.tableName) \
                      .get(key) \
                      .delete() \
                      .run(self.conn)
    
                # self.assertRaises(Exception, feed.next)

            value = next(self._generator)
            document = self._document(value)
            key = self.r.db(self.dbName) \
                        .table(self.tableName) \
                        .insert(document, return_changes=True) \
                        .run(self.conn) \
                        .get("generated_keys", [document.get("id", value)])[0]

            changes = 1
            if self._multi:
                changes = min(self._multi_len, self._limit)

            for x in xrange(changes):
                next(feed)

            # self.assertRaises(Exception, feed.next)

            self.r.db(self.dbName) \
                  .table(self.tableName) \
                  .get(key) \
                  .delete() \
                  .run(self.conn)

            for x in xrange(changes):
                feed_next = next(feed)

                self.assertTrue("old_val" in feed_next)
                self.assertTrue("new_val" in feed_next)
                self.assertTrue(feed_next["old_val"][self._field] < value or (
                                    feed_next["old_val"][self._field] == value and
                                    feed_next["old_val"]["id"] <= key))
                if len(initial) + x < self._limit:
                    self.assertEqual(feed_next["new_val"], None)

            # self.assertRaises(Exception, feed.next)

    def test_replace_key(self):
        # FIXME: Python 2.7 has new facilities allowing tests to be skipped, use those
        # when we no longer need to support 2.6
        if self._field == self._primary_key:
            # The primary key can not be updated, skip it
            return

        query = self.r.db(self.dbName) \
                      .table(self.tableName) \
                      .order_by(index=self.r.desc(self._field)) \
                      .limit(self._limit) \
                      .changes(squash=self._squash)

        with utils.NextWithTimeout(query.run(self._feed_conn), stopOnEmpty=False) as feed:
            changes = min(self._records, self._limit)
            if self._multi:
                changes = min(
                    self._records * self._multi_len, self._limit)

            initial = []
            for x in xrange(changes): 
                initial.append(next(feed))

            # self.assertRaises(Exception, feed.next)

            index, value = self._generator_initial.pop()
            document = self._document(value, "g-%i" % index)
            key = self.r.db(self.dbName) \
                      .table(self.tableName) \
                      .insert(document, return_changes=True) \
                      .run(self.conn) \
                      .get("generated_keys", [document.get("id", value)])[0]

            changes = 0
            if len(initial) < self._limit:
                changes = 1
                if self._multi:
                    changes = min(self._multi_len, self._limit - len(initial))

            for x in xrange(changes):
                feed_next = next(feed)

                self.assertTrue("old_val" in feed_next)
                self.assertTrue("new_val" in feed_next)
                if len(initial) + x < self._limit:
                    self.assertEqual(feed_next["old_val"], None)
                self.assertEqual(feed_next["new_val"]["id"], key)
                self.assertEqual(feed_next["new_val"][self._field], value)

            # self.assertRaises(Exception, feed.next)

            update = next(self._generator)
            self.r.db(self.dbName) \
                  .table(self.tableName) \
                  .get(key) \
                  .update({
                      self._field: update
                  }) \
                  .run(self.conn)

            changes = 1
            if self._multi:
                changes = min(self._multi_len, self._limit)

            for x in xrange(changes):
                feed_next = next(feed)

                self.assertTrue("old_val" in feed_next)
                self.assertTrue("new_val" in feed_next)
                self.assertTrue(
                    feed_next["old_val"][self._field] <= feed_next["new_val"][self._field])
                self.assertEqual(feed_next["new_val"]["id"], key)
                self.assertEqual(feed_next["new_val"][self._field], update)

            # self.assertRaises(Exception, feed.next)

            self.r.db(self.dbName) \
                  .table(self.tableName) \
                  .get(key) \
                  .update({
                      self._field: value
                  }) \
                  .run(self.conn)

            changes = 1
            if self._multi:
                changes = min(self._multi_len, self._limit)

            for x in xrange(changes):
                feed_next = next(feed)

                self.assertTrue("old_val" in feed_next)
                self.assertTrue("new_val" in feed_next)
                self.assertTrue(feed_next["old_val"][self._field] <= update)
                self.assertTrue(feed_next["new_val"][self._field] >= value)

            # self.assertRaises(Exception, feed.next)

    def bare_test_squash_to_nothing_insert_delete(self):
        # FIXME: Python 2.7 has new facilities allowing tests to be skipped, use those
        # when we no longer need to support 2.6
        if self._squash == True:
            # This is too unpredictable
            return

        query = self.r.db(self.dbName) \
                    .table(self.tableName) \
                    .order_by(index=self.r.desc(self._field)) \
                    .limit(self._limit) \
                    .changes(squash=self._squash)

        with utils.NextWithTimeout(query.run(self._feed_conn), stopOnEmpty=False) as feed:
            changes = min(self._records, self._limit)
            if self._multi:
                changes = min(
                    self._records * self._multi_len, self._limit)

            initial = []
            for x in xrange(changes): 
                initial.append(next(feed))

            value = next(self._generator)
            key = self.r.db(self.dbName) \
                      .table(self.tableName) \
                      .insert(self._document(value, key_generate=False), return_changes=True) \
                      .run(self.conn).get("generated_keys", [value])[0]

            self.r.db(self.dbName) \
                  .table(self.tableName) \
                  .get(key) \
                  .delete() \
                  .run(self.conn)

            # self.assertRaises(Exception, feed.next)

    def test_squash_to_nothing_delete_insert(self):
        # This test is similar to the one above but must be done in a separate function
        # due to timing issues

        # FIXME: Python 2.7 has new facilities allowing tests to be skipped, use those
        # when we no longer need to support 2.6
        if self._squash == True:
            # This is too unpredictable
            return

        query = self.r.db(self.dbName) \
                    .table(self.tableName) \
                    .order_by(index=self.r.desc(self._field)) \
                    .limit(self._limit) \
                    .changes(squash=self._squash)

        with utils.NextWithTimeout(query.run(self._feed_conn), stopOnEmpty=False) as feed:
            changes = min(self._records, self._limit)
            if self._multi:
                changes = min(
                    self._records * self._multi_len, self._limit)

            initial = []
            for x in xrange(changes): 
                initial.append(next(feed))

            if len(initial):
                self.r.db(self.dbName) \
                      .table(self.tableName) \
                      .get(initial[0]["new_val"]["id"]) \
                      .delete() \
                      .run(self.conn)

                self.r.db(self.dbName) \
                      .table(self.tableName) \
                      .insert(initial[0]["new_val"]) \
                      .run(self.conn)

                # self.assertRaises(Exception, feed.next)


class MultiGenerator(object):
    def __init__(self):
        self._count = itertools.count(3)

    def __iter__(self):
        return self

    def __next__(self):
        return self.next()

    def next(self):
        # This is crafted to be predictable, imagine you have an initial set
        # [
        #     {u'new_val': {u'insert': True, u'multi': [44, -1], u'id': u'g-20'}},
        #     {u'new_val': {u'insert': True, u'multi': [45, -1], u'id': u'g-21'}},
        #     {u'new_val': {u'insert': True, u'multi': [46, -1], u'id': u'g-22'}},
        #     {u'new_val': {u'insert': True, u'multi': [47, -1], u'id': u'g-23'}},
        #     {u'new_val': {u'insert': True, u'multi': [48, -1], u'id': u'g-24'}},
        # ->
        #     {u'new_val': {u'insert': True, u'multi': [44, -1], u'id': u'g-20'}},
        #     {u'new_val': {u'insert': True, u'multi': [45, -1], u'id': u'g-21'}},
        #     {u'new_val': {u'insert': True, u'multi': [46, -1], u'id': u'g-22'}},
        #     {u'new_val': {u'insert': True, u'multi': [47, -1], u'id': u'g-23'}},
        #     {u'new_val': {u'insert': True, u'multi': [48, -1], u'id': u'g-24'}}
        # ]
        # and want to insert the document
        #     {'insert': True, 'multi': [43, -1], u'id': 'g-19'}.
        #
        # This will get inserted once in the position marked by the arrow, which is
        # hard to calculate or predict.

        return [self._count.next()] * 3
