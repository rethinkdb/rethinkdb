#!/usr/bin/env python
# Copyright 2010-2015 RethinkDB, all rights reserved.

import os, sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import rdb_workload_common
from vcoptparse import *

op = rdb_workload_common.option_parser_for_connect()
op["n_appends"] = IntFlag("--num-appends", 20000)
opts = op.parse(sys.argv)

with rdb_workload_common.make_table_and_connection(opts) as (table, conn):

    key = 'fizz'
    val_chunks = ['buzzBUZZZ', 'baazBAAZ', 'bozoBOZO']

    table.insert({'id': key, 'val': val_chunks[0]}).run(conn)

    for i in xrange(1, opts["n_appends"]):
        response = table.get(key).update(lambda row: {'val': row['val'].add(val_chunks[i % 3])}, durability="soft").run(conn)

    # Read the reply from the last command.
    if (response['replaced'] != 1):
        raise ValueError("Expecting { replaced: 1 } in reply.")

    expected_val = ''.join([val_chunks[i % 3] for i in xrange(opts["n_appends"])])

    actual_val = table.get(key)['val'].run(conn)
    
    if (expected_val != actual_val):
        print "Expected val: %s" % expected_val
        print "Incorrect val (len=%d): %s" % (len(actual_val), actual_val)
        raise ValueError("Incorrect value.")
