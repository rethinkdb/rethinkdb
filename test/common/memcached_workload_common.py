# This is a (hopefully temporary) shim that uses the rdb protocol to
# implement part of the memcache API

import contextlib
import rdb_workload_common

@contextlib.contextmanager
def make_memcache_connection(opts):
    with rdb_workload_common.make_table_and_connection(opts) as (table, conn):
        yield MemcacheRdbShim(table, conn)

class MemcacheRdbShim(object):
    def __init__(self, table, conn):
        self.table = table
        self.conn = conn

    def get(self, key):
        response = self.table.get(key).run(self.conn)
        if response:
            return response['val']

    def set(self, key, val):
        response = self.table.insert({
            'id': key,
            'val': val
            },
            upsert=True
            ).run(self.conn)

        error = response.get('first_error')
        if error:
            raise Exception(error)

        return response['inserted'] | response['replaced'] | response['unchanged']

    def delete(self, key):
        response = self.table.get(key).delete().run(self.conn)

        error = response.get('first_error')
        if error:
            raise Exception(error)

        return response['deleted']


def option_parser_for_memcache():
    return rdb_workload_common.option_parser_for_connect()
