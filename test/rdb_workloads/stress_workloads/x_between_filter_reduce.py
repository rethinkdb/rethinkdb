#!/usr/bin/env python
import sys, os, datetime, pareto

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', 'drivers', 'python')))
import rethinkdb as r

class Workload:
    def __init__(self, options):
        self.db = options["db"]
        self.table = options["table"]
        self.cid_dist = pareto.Pareto(1000)
        self.typ_dist = pareto.Pareto(10)

    def run(self, conn):
        cid = "customer%03d" % self.cid_dist.get()
        typ = "type%d" % self.typ_dist.get()

        # Figure out which year/month we're checking
        month_delta = pareto.shitty_power_law(0.8)
        date = datetime.date.today()

        while date.month < month_delta:
            month_delta -= date.month
            date.month = 12
            date.year -= 1

        time_1 = r.time(date.year, date.month, 1, 'Z')
        time_2 = r.time(date.year, date.month + 1, 1, 'Z')

        # TODO: get this working
        #                                    .reduce(lambda acc, row: acc + row["arr"].reduce(lambda acc2, val: acc2 + val, 0), 0) \
        res = r.db(self.db).table(self.table).between([cid, time_1], [cid, time_2], index="compound") \
                                             .filter(lambda row: row["type"].eq(typ)) \
                                             .count() \
                                             .run(conn)

        return { }
