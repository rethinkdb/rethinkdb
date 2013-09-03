#!/usr/bin/env python
import sys, os, datetime, pareto

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', 'drivers', 'python')))
import rethinkdb as r

class Workload:
    def __init__(self, options):
        self.db = options["db"]
        self.table = options["table"]

    def run(self, conn):
        # Figure out which year/month we're checking
        month_delta = pareto.shitty_power_law(0.8)
        date = datetime.date.today()

        while date.month < month_delta:
            month_delta -= date.month
            date.month = 12
            date.year -= 1

        time_1 = r.time(date.year, date.month, 1, 'Z')
        time_2 = r.time(date.year, date.month + 1, 1, 'Z')

        cursor = r.db(self.db).table(self.table).between(time_1, time_2, index="datetime").count().run(conn)

        return { }
