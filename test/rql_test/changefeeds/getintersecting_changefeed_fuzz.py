#!/usr/bin/env python
import datetime
import os
import random
import re
import sys
import tempfile
import time
import traceback
import unittest
import functools
import socket
from tornado import gen, ioloop
from tornado.concurrent import Future

sys.path.insert(0, os.path.join(os.path.dirname(os.path.realpath(__file__)),
                                os.pardir, "common"))
import utils

r = utils.import_python_driver()
r.set_loop_type("tornado")

@gen.coroutine
def open_connection():
    if len(sys.argv) > 1:
        port = int(sys.argv[0])
    else:
        port = int(os.getenv('RDB_DRIVER_PORT'))

    conn = yield r.connect(port=port)
    raise gen.Return(conn)

@gen.coroutine
def random_geo(conn):
    """
    Returns a Future of a ReQL query object representing a random, valid geographic
    object, which is a point, line, or polygon. Uses the connection given to verify
    the validity of the geometry before returning it.
    """

    def random_point():
        return r.point(random.uniform(-180, 180), random.uniform(-90, 90))

    class_random = random.random()
    if class_random < 0.5:
        # points need no verification
        raise gen.Return(random_point())
    elif class_random < 0.7:
        # lines need no verification
        args = [random_point() for i in range(random.randint(2, 8))]
        raise gen.Return(r.line(r.args(args)))
    else:
        # polygons must obey certain rules, such as not being self-intersecting. we use
        # the server to verify the validity of our (occasionally invalid) polygons.

        # because circles are implemented as polygons and are easier to reason about,
        # we use them.

        while True:
            shell_lat = random.uniform(-180, 180)
            shell_lon = random.uniform(-90, 90)
            shell_radius = random.uniform(0, 100000) # 100km ought to be enough for anyone

            poly = r.circle(r.point(shell_lat, shell_lon), shell_radius)
            try:
                yield poly.run(conn)
            except r.ReqlRuntimeError:
                continue

            if random.random() < 0.5:
                # try to add a hole. occasionally, these holes will be outside of the
                # shell, so we try repeatedly.

                # 112353 meters ~= 1 degree at equator, the largest it could be
                radius_in_deg = shell_radius / 112353
                while True:
                    hole_lat = shell_lat + random.uniform(-radius_in_deg, radius_in_deg)
                    hole_lon = shell_lon + random.uniform(-radius_in_deg, radius_in_deg)
                    hole_radius = random.uniform(0, shell_radius)

                    hole = r.circle(r.point(hole_lat, hole_lon), hole_radius)
                    try:
                        yield poly.polygon_sub(hole).run(conn)
                    except r.ReqlRuntimeError:
                        continue
                    else:
                        poly = poly.polygon_sub(hole)
                        break

            raise gen.Return(poly)

class DatasetTracker(object):
    """
    Runs a get_intersecting changefeed on a table given a certain query geometry
    and tracks its contents.

    You may call check_validity when no writes are occuring concurrently to verify
    that the changefeed results are the same as the results of a non-changefeed
    get_intersecting query for the same query geometry.
    """

    def __init__(self, conn, table, query_geo, id):
        self.known_objects = dict()
        self.last_timestamp = time.time()
        self.state = 'initializing'
        self.deliberately_closed = False
        self.query_geo = query_geo
        self.id = id
        self.table = table
        self.conn = conn

        self._process_query_results()

    def close(self):
        if 'changefeed_cursor' not in self:
            raise Exception("closed before processing query results")
        self.deliberately_closed = True
        self.changefeed_cursor.close()

    @gen.coroutine
    def _process_query_results(self):
        # Small batch size to get more interesting interactions in the splice_stream_t.
        cur = yield r.table(self.table).get_intersecting(self.query_geo, index='g').changes(include_initial=True, include_states=True).run(self.conn, max_batch_rows=1)
        self.changefeed_cursor = cur

        while (yield cur.fetch_next()):
            value = yield cur.next()
            self.last_timestamp = time.time()

            if 'error' in value:
                raise Exception(self.id + " Got error in cursor: "+value['error'])
            elif 'state' in value:
                print(self.id + " State change: " + value['state'])
                self.state = value['state']

            if value.get('old_val') is not None:
                print(self.id + " Change: Removing " + value['old_val']['id'])
                del self.known_objects[value['old_val']['id']]

            if value.get('new_val') is not None:
                print(self.id + " Change: Adding " + value['new_val']['id'])
                if value['new_val']['id'] in self.known_objects:
                    raise Exception(self.id + " Duplicate value for ID "+value['new_val']['id'])
                self.known_objects[value['new_val']['id']] = value['new_val']

        if not self.deliberately_closed:
            raise Exception(self.id + " Fell off end of supposedly infinite cursor")

    @gen.coroutine
    def wait_quiescence(self):
        # first, wait a long period of time so that items can start to arrive
        yield gen.sleep(0.5)

        # then wait a short period of time repeatedly until items have
        # stopped arriving
        while time.time() - self.last_timestamp < 0.5:
            yield gen.sleep(0.1)

    @gen.coroutine
    def check_validity(self):
        yield self.wait_quiescence()

        start_timestamp = self.last_timestamp

        have = dict()
        cur = yield r.table(self.table).get_intersecting(self.query_geo, index='g').run(self.conn)
        while (yield cur.fetch_next()):
            value = yield cur.next()
            have[value['id']] = value

        if start_timestamp != self.last_timestamp:
            raise Exception("Got values in changefeed after it had supposedly quiesced")

        if have != self.known_objects:
            raise Exception("Invalid objects. Got %r from query, %r from changefeed" % (have, self.known_objects))

@gen.coroutine
def make_changes(conn, table, change_count):
    """
    Makes random insertions, deletions, and updates to values in the table given.
    """

    ids = set()
    
    cur = yield r.table(table).run(conn)
    while (yield cur.fetch_next()):
        value = yield cur.next()
        ids.add(value['id'])

    for i in range(change_count):
        optype_random = random.random()

        if optype_random < 0.2:
            # insert
            g = yield random_geo(conn)
            print ("Inserting")
            res = yield r.table(table).insert({"g": g}).run(conn)
            if res['inserted'] != 1:
                raise Exception("wanted inserted=1, got %r" % res)
            ids.add(res['generated_keys'][0])

        elif optype_random < 0.4:
            # delete
            if len(ids) == 0:
                break

            print ("Deleting")
            delete_id = random.sample(ids, 1)[0]
            res = yield r.table(table).get(delete_id).delete().run(conn)
            if res['deleted'] != 1 and res['skipped'] != 1:
                raise Exception("wanted deleted=1, got %r" % res)

        else:
            # update existing
            if len(ids) == 0:
                break
            g = yield random_geo(conn)
            # We want to do an atomic update, so we evaluate the geometry first.
            g = yield g.run(conn)
            update_id = random.sample(ids, 1)[0]
            print ("Updating")
            res = yield r.table(table).get(update_id).update({"g": g}).run(conn)
            if res['replaced'] != 1 and res['skipped'] != 1:
                raise Exception("wanted replaced=1, got %r" % res)

@gen.coroutine
def main():
    conn = yield open_connection()

    if "test" in (yield r.table_list().run(conn)):
        yield r.table_drop("test").run(conn)
    yield r.table_create("test").run(conn)
    yield r.table("test").index_create("g", geo=True).run(conn)
    yield r.table("test").index_wait("g").run(conn)

    print("Starting validators")

    validators = []
    for i in range(50):
        query_geo = yield random_geo(conn)
        validators.append(DatasetTracker(conn, "test", query_geo, "inititial-" + str(i)))

    for i in range(20):
        print("Making changes")
        yield make_changes(conn, "test", random.randint(5,100))
        print("Validating")
        yield [v.check_validity() for v in validators]
        print("Adding new validator")
        query_geo = yield random_geo(conn)
        validators.append(DatasetTracker(conn, "test", query_geo, "late-" + str(i)))
    print("All done!")

if __name__ == '__main__':
    ioloop.IOLoop.current().run_sync(main)
