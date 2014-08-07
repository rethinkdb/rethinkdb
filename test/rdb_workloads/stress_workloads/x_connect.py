#!/usr/bin/env python
import sys, os, random

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import utils
r = utils.import_python_driver()

class Workload:
    def __init__(self, options):
        self.db = options["db"]
        self.table = options["table"]
        self.hosts = options["hosts"]

    def run(self, conn):
        host = random.choice(self.hosts)
        with r.connect(host[0], host[1]) as conn:
            pass
        return {}
