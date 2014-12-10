#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

from __future__ import print_function

import os, random, sys, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import rdb_workload_common, vcoptparse

op = rdb_workload_common.option_parser_for_connect()
op["num_keys"] = vcoptparse.IntFlag("--num-keys", 5000)
op["sequential"] = vcoptparse.BoolFlag("--sequential")
op["phase"] = vcoptparse.ChoiceFlag("--phase", ["w", "r", "wr"], "wr")
opts = op.parse(sys.argv)

with rdb_workload_common.make_table_and_connection(opts) as (table, conn):

    if "w" in opts["phase"]:
        print("Inserting (%.2fs)" % (time.time() - startTime))
        keys = [str(x) for x in xrange(opts["num_keys"])]
        if not opts["sequential"]:
            random.shuffle(keys)
        i = 0
        for key in keys:
            response = table.insert({'id': key, 'val': key}).run(conn)
            if response['inserted'] != 1:
                raise ValueError("Could not set %r" % key)
            i += 1
        if "r" not in opts["phase"]:
            print("Dumping chosen keys to disk (%.2fs)" % (time.time() - startTime))
            with open("keys", "w") as keys_file:
                keys_file.write(" ".join(keys))

    if "r" in opts["phase"]:
        if "w" not in opts["phase"]:
            print("Loading chosen keys from disk (%.2fs)" % (time.time() - startTime))
            with open("keys", "r") as keys_file:
                keys = keys_file.read().split(" ")
        # Verify everything
        print("Verifying (%.2fs)" % (time.time() - startTime))
        i = 0
        for key in keys:
            if i % 16 == 0:
                response = table.get_all(*[str(k) for k in keys[i:i + 16]]).run(conn)
                values = dict((row['id'], row['val']) for row in response)
            value = values[str(key)]
            if value != key:
                raise ValueError("Key %r is set to %r, expected %r" % (key, value, key))
            i += 1

    print("Success (%.2fs)" % (time.time() - startTime))
