#!/usr/bin/env python
import sys, os, x_stress_util

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import utils
r = utils.import_python_driver()

class Workload:
    def __init__(self, options):
        self.db = options["db"]
        self.table = options["table"]
        self.cid_dist = x_stress_util.Pareto(1000)

    def run(self, conn):
        cid = "customer%03d" % self.cid_dist.get()
        cursor = r.db(self.db).table(self.table).get_all(cid, index="customer_id").group("type").count().run(conn)

        for row in cursor:
            pass

        return {}
