#!/usr/bin/env python
import sys, os, datetime, x_stress_util

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import utils
r = utils.import_python_driver()

class Workload:
    def __init__(self, options):
        self.db = options["db"]
        self.table = options["table"]
        self.time_dist = x_stress_util.TimeDistribution(os.getenv("X_END_DATE"), os.getenv("X_DATE_INTERVAL"))

    def run(self, conn):
        (start_date, end_date) = self.time_dist.get()

        time_1 = r.time(start_date.year, start_date.month, start_date.day, 'Z')
        time_2 = r.time(end_date.year, end_date.month, end_date.day, 'Z')

        cursor = r.db(self.db).table(self.table).between(time_1, time_2, index="datetime").count().run(conn)

        return {}
