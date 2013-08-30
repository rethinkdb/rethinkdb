#!/usr/bin/env python
import md5, sys, os, time, errno
from pareto import Pareto

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', 'drivers', 'python')))
import rethinkdb as r

class Workload:
    def __init__(self, options):
        self.db = options["db"]
        self.table = options["table"]
        self.key_dist = Pareto(options["max_key"])

    def run(self, conn):
        original_key = self.key_dist.get()
        key = md5.new(str(original_key)).hexdigest()
        rql_res = r.db(self.db).table(self.table).get(key).run(conn)

        if rql_res is None:
            return { "errors": [ "key not found: %d" % original_key ] }

        return { }
