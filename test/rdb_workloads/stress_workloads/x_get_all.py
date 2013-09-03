#!/usr/bin/env python
import sys, os, pareto

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', 'drivers', 'python')))
import rethinkdb as r

class Workload:
    def __init__(self, options):
        self.db = options["db"]
        self.table = options["table"]
        self.cid_dist = pareto.Pareto(1000)

    def run(self, conn):
        cid = "customer%03d" % self.cid_dist.get()
        cursor = r.db(self.db).table(self.table).get_all(cid, index="customer_id").group_by("type", r.count).run(conn)

        for row in cursor:
            print row
            pass

        return { }
