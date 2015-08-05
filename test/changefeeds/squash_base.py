#!/usr/bin/env python
# Copyright 2014-2015 RethinkDB, all rights reserved.

import itertools, os, sys, time

try:
    xrange
except NameError:
    xrange = range

sys.path.append(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common'))
import rdb_unittest, utils

# ---

class SquashBase(rdb_unittest.RdbTestCase):
    '''Squash tests'''
    
    # Local variables
    squash = True
    field = "id"
    generator = itertools.count()
    records = 0
    limit = 0
    multi = False

    def setUp(self):
        super(SquashBase, self).setUp()

        # The generator emits values in increasing order thus we store the first twenty
        # values for later use, specifically to do inserts and updates, and test that no
        # change is emitted.
        self._generator_initial_len = 20
        self._generator_initial = []
        for x in xrange(self._generator_initial_len):
            self._generator_initial.append((x, next(self.generator)))

        # The primary key is used to break ties in a multi index thus `self._document`
        # has the option of generating an increasing key instead of them being
        # auto-generated.
        self._key_generator = itertools.count(self._generator_initial_len)
        if self.multi:
            # The generator for multi indices return an array, and the length of that
            # array is a factor in the number of results from a changefeed.
            self._multi_len = len(self._generator_initial[0][1])

        self._primary_key = self.r.db(self.dbName) \
                                  .table(self.tableName) \
                                  .info()["primary_key"] \
                                  .run(self.conn)

        # Generate the records ..
        for x in xrange(self.records):
            self.r.db(self.dbName) \
                  .table(self.tableName) \
                  .insert(self._document(next(self.generator))) \
                  .run(self.conn)

        # .. and add the requested index if necessary
        if self.field != self._primary_key:
            self.r.db(self.dbName) \
                  .table(self.tableName) \
                  .index_create(self.field, multi=self.multi) \
                  .run(self.conn)

            self.r.db(self.dbName) \
                  .table(self.tableName) \
                  .index_wait(self.field) \
                  .run(self.conn)

        # The changefeeds are requested through a separate connection
        self._feed_conn = self.r.connect(
            self.cluster[0].host, self.cluster[0].driver_port) 

    def _document(self, value, key=None, key_generate=None):
        # An increasing primary key is automatically added to multi indices as they
        # influence sorting.
        if key_generate is None:
            key_generate = self.multi

        document = {
            self.field: value
        }

        if key is None and key_generate:
            key = "g-%i" % next(self._key_generator)

        if key is not None:
            self.assertTrue(self.field != self._primary_key)
            document[self._primary_key] = key

        return document

    def test_insert(self):
        query = self.r.db(self.dbName) \
                      .table(self.tableName) \
                      .order_by(index=self.r.desc(self.field)) \
                      .limit(self.limit) \
                      .changes(squash=self.squash)

        with utils.NextWithTimeout(query.run(self._feed_conn), stopOnEmpty=False) as feed:
            changes = min(self.records, self.limit)
            if self.multi:
                changes = min(
                    self.records * self._multi_len, self.limit)

            initial = []
            for x in xrange(changes):
                initial.append(next(feed))

            # If the number of records is greater than the limit then then insert a low
            # value and verify it does not show up as a change, due to the `order_by`.
            if self.records >= self.limit:
                _, value = self._generator_initial.pop()
                self.r.db(self.dbName) \
                      .table(self.tableName) \
                      .insert(self._document(value)) \
                      .run(self.conn)
                
                self.assertRaises(Exception, feed.next)

            # Insert a value and verify it does show up.
            value = next(self.generator)
            document = self._document(value)
            key = self.r.db(self.dbName) \
                        .table(self.tableName) \
                        .insert(document, return_changes=True) \
                        .run(self.conn) \
                        .get("generated_keys", [document.get("id", value)])[0]

            # With multi indices a single document may show up multiple times in the
            # changefeed, `changes` calculates the number to verify it does indeed show
            # up the expected number of times.
            changes = 1
            if self.multi:
                changes = min(self._multi_len, self.limit)

            for x in xrange(changes):
                feed_next = next(feed)

                self.assertTrue("old_val" in feed_next)
                self.assertTrue("new_val" in feed_next)
                # It depends on whether the initial limit was fulfilled whether the
                # change has an "old_val" set to `None` or a document.
                if len(initial) + x >= self.limit:
                    # Note that the initial values are ordered descending, hence
                    # the comparison with initial[-(x + 1)]
                    self.assertEqual(
                        feed_next["old_val"][self.field],
                        initial[-(x + 1)]["new_val"][self.field])
                else:
                    self.assertEqual(feed_next["old_val"], None)
                self.assertEqual(feed_next["new_val"]["id"], key)
                self.assertEqual(feed_next["new_val"][self.field], value)

    def test_insert_batch(self):
        # FIXME: Python 2.7 has new facilities allowing tests to be skipped, use those
        # when we no longer need to support 2.6
        if self.squash == True:
            # With squash True it might not squash agressively enough for this to be
            # predictable, skip it
            return

        query = self.r.db(self.dbName) \
                      .table(self.tableName) \
                      .order_by(index=self.r.desc(self.field)) \
                      .limit(self.limit) \
                      .changes(squash=self.squash)

        with utils.NextWithTimeout(query.run(self._feed_conn), stopOnEmpty=False) as feed:
            changes = min(self.records, self.limit)
            if self.multi:
                changes = min(
                    self.records * self._multi_len, self.limit)

            initial = []
            for x in xrange(changes): 
                initial.append(next(feed))

            # Insert two more documents than the limit as a single batch, due to the
            # squashing we should never get a change for the first two.
            documents = []
            for x in xrange(self.limit + 2):
                value = next(self.generator)
                if self.field == self._primary_key:
                    documents.append(self._document(value))
                else:
                    documents.append(self._document(value, key=x))

            # A document with duplicate primary key should be ignored as well.
            error = documents[-1].copy()
            error.update({"error": True})
            documents.append(error)

            self.r.db(self.dbName) \
                  .table(self.tableName) \
                  .insert(documents) \
                  .run(self.conn)

            for x in xrange(self.limit):
                feed_next = next(feed)

                self.assertTrue("old_val" in feed_next)
                self.assertTrue("new_val" in feed_next)
                if len(initial) + x >= self.limit:
                    self.assertTrue(
                        feed_next["old_val"] in map(lambda x: x["new_val"], initial))
                else:
                    self.assertEqual(feed_next["old_val"], None)
                self.assertTrue(feed_next["new_val"] in documents[2:])
                self.assertTrue(not "error" in feed_next["new_val"])

    def test_delete(self):
        query = self.r.db(self.dbName) \
                      .table(self.tableName) \
                      .order_by(index=self.r.desc(self.field)) \
                      .limit(self.limit) \
                      .changes(squash=self.squash) 

        with utils.NextWithTimeout(query.run(self._feed_conn), stopOnEmpty=False) as feed:
            changes = min(self.records, self.limit)
            if self.multi:
                changes = min(
                    self.records * self._multi_len, self.limit)

            initial = []
            for x in xrange(changes): 
                initial.append(next(feed))

            # If the number of records is greater than the limit then insert and
            # subsequently delete a low value, and verify it does not show up as a
            # change because of the `order_by`.
            if self.records >= self.limit:
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
    
            # In inserting this document we have to do somewhat of a dance to get its
            # primary key as it might either be the field, generated by us because of a
            # multi index, or auto-generated.
            value = next(self.generator)
            document = self._document(value)
            key = self.r.db(self.dbName) \
                        .table(self.tableName) \
                        .insert(document, return_changes=True) \
                        .run(self.conn) \
                        .get("generated_keys", [document.get("id", value)])[0]

            changes = 1
            if self.multi:
                changes = min(self._multi_len, self.limit)

            for x in xrange(changes):
                next(feed)

            # With the primary key delete the record again.
            self.r.db(self.dbName) \
                  .table(self.tableName) \
                  .get(key) \
                  .delete() \
                  .run(self.conn)

            for x in xrange(changes):
                feed_next = next(feed)

                self.assertTrue("old_val" in feed_next)
                self.assertTrue("new_val" in feed_next)
                self.assertTrue(feed_next["old_val"][self.field] < value or (
                                    feed_next["old_val"][self.field] == value and
                                    feed_next["old_val"]["id"] <= key))
                if len(initial) + x < self.limit:
                    self.assertEqual(feed_next["new_val"], None)

    def test_replace_key(self):
        # FIXME: Python 2.7 has new facilities allowing tests to be skipped, use those
        # when we no longer need to support 2.6
        if self.field == self._primary_key:
            # The primary key can not be updated, skip it
            return

        query = self.r.db(self.dbName) \
                      .table(self.tableName) \
                      .order_by(index=self.r.desc(self.field)) \
                      .limit(self.limit) \
                      .changes(squash=self.squash)

        with utils.NextWithTimeout(query.run(self._feed_conn), stopOnEmpty=False) as feed:
            changes = min(self.records, self.limit)
            if self.multi:
                changes = min(
                    self.records * self._multi_len, self.limit)

            initial = []
            for x in xrange(changes): 
                initial.append(next(feed))

            # Insert a low value, this may or may not cause changes depending on whether
            # we've had more initial changes than the limit.
            index, value = self._generator_initial.pop()
            document = self._document(value, "g-%i" % index)
            key = self.r.db(self.dbName) \
                      .table(self.tableName) \
                      .insert(document, return_changes=True) \
                      .run(self.conn) \
                      .get("generated_keys", [document.get("id", value)])[0]

            changes = 0
            if len(initial) < self.limit:
                changes = 1
                if self.multi:
                    changes = min(self._multi_len, self.limit - len(initial))

            for x in xrange(changes):
                feed_next = next(feed)

                self.assertTrue("old_val" in feed_next)
                self.assertTrue("new_val" in feed_next)
                if len(initial) + x < self.limit:
                    self.assertEqual(feed_next["old_val"], None)
                self.assertEqual(feed_next["new_val"]["id"], key)
                self.assertEqual(feed_next["new_val"][self.field], value)

            # Update the key to a higher value, this should produce a change (or changes
            # in the case of a multi index).
            update = next(self.generator)
            self.r.db(self.dbName) \
                  .table(self.tableName) \
                  .get(key) \
                  .update({
                      self.field: update
                  }) \
                  .run(self.conn)

            changes = 1
            if self.multi:
                changes = min(self._multi_len, self.limit)

            for x in xrange(changes):
                feed_next = next(feed)

                self.assertTrue("old_val" in feed_next)
                self.assertTrue("new_val" in feed_next)
                self.assertTrue(
                    feed_next["old_val"][self.field] <= feed_next["new_val"][self.field])
                self.assertEqual(feed_next["new_val"]["id"], key)
                self.assertEqual(feed_next["new_val"][self.field], update)

            # Update the key back to the lower value.
            self.r.db(self.dbName) \
                  .table(self.tableName) \
                  .get(key) \
                  .update({
                      self.field: value
                  }) \
                  .run(self.conn)

            changes = 1
            if self.multi:
                changes = min(self._multi_len, self.limit)

            for x in xrange(changes):
                feed_next = next(feed)

                self.assertTrue("old_val" in feed_next)
                self.assertTrue("new_val" in feed_next)
                self.assertTrue(feed_next["old_val"][self.field] <= update)
                self.assertTrue(feed_next["new_val"][self.field] >= value)

    def bare_test_squash_to_nothing_insert_delete(self):
        # FIXME: Python 2.7 has new facilities allowing tests to be skipped, use those
        # when we no longer need to support 2.6
        if self.squash == True:
            # This is too unpredictable
            return

        query = self.r.db(self.dbName) \
                    .table(self.tableName) \
                    .order_by(index=self.r.desc(self.field)) \
                    .limit(self.limit) \
                    .changes(squash=self.squash)

        with utils.NextWithTimeout(query.run(self._feed_conn), stopOnEmpty=False) as feed:
            changes = min(self.records, self.limit)
            if self.multi:
                changes = min(
                    self.records * self._multi_len, self.limit)

            initial = []
            for x in xrange(changes): 
                initial.append(next(feed))

            # An insert followed by a delete within a two-second squashing period should
            # not lead to a change being emitted.
            value = next(self.generator)
            key = self.r.db(self.dbName) \
                      .table(self.tableName) \
                      .insert(self._document(value, key_generate=False), return_changes=True) \
                      .run(self.conn).get("generated_keys", [value])[0]

            self.r.db(self.dbName) \
                  .table(self.tableName) \
                  .get(key) \
                  .delete() \
                  .run(self.conn)

    def test_squash_to_nothing_delete_insert(self):
        # This test is similar to the one above but must be done in a separate function
        # due to timing issues

        # FIXME: Python 2.7 has new facilities allowing tests to be skipped, use those
        # when we no longer need to support 2.6
        if self.squash == True:
            # This is too unpredictable
            return

        query = self.r.db(self.dbName) \
                    .table(self.tableName) \
                    .order_by(index=self.r.desc(self.field)) \
                    .limit(self.limit) \
                    .changes(squash=self.squash)

        with utils.NextWithTimeout(query.run(self._feed_conn), stopOnEmpty=False) as feed:
            changes = min(self.records, self.limit)
            if self.multi:
                changes = min(
                    self.records * self._multi_len, self.limit)

            initial = []
            for x in xrange(changes): 
                initial.append(next(feed))

            # As above, deleting and re-inserting a value should not lead to a change
            # being emitted.
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
