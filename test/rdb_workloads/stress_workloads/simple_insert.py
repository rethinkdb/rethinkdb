#!/usr/bin/env python

# This stress workload inserts small documents of a random size under 256 bytes.

import math, sys, os, random

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', 'drivers', 'python')))
import rethinkdb as r

class Workload:
    def __init__(self, options):
        self.db = options["db"]
        self.table = options["table"]

    def run(self, conn):

        doc = {'str': 'X' * random.randint(1, 200)}

        write_result = r.db(self.db).table(self.table).insert(doc).run(conn)

        if write_result['inserted'] != 1:
            return {"errors": ["Insert failed: " + str(write_result)]}

        return {}
