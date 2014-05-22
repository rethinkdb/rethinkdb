#!/usr/bin/env python
import sys, socket, random, time, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import rdb_workload_common
from vcoptparse import *

op = rdb_workload_common.option_parser_for_connect()
op["count"] = IntFlag("--count", 10000)
opts = op.parse(sys.argv)

if __name__ == '__main__':
    with rdb_workload_common.make_table_and_connection(opts) as (table, conn):
        rdb_workload_common.insert_many(conn=conn, table=table, count=opts['count'])
