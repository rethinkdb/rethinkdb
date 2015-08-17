#!/usr/bin/env python
import md5, sys, os, errno, x_stress_util

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import utils
r = utils.import_python_driver()

class Workload:
    def __init__(self, options):
        self.db = options["db"]
        self.table = options["table"]
        self.count = 1

        # Get the current max key from environment variables
        max_key = os.getenv("X_MAX_KEY")
        max_key_file = os.getenv("X_MAX_KEY_FILE")

        if max_key is None:
            if max_key_file is None:
                raise RuntimeError("X_MAX_KEY and X_MAX_KEY_FILE are both undefined")
            elif not os.path.isfile(max_key_file):
                raise RuntimeError("X_MAX_KEY not defined, and key file '%s' does not exist" % max_key_file)
            else:
                with open(max_key_file, "r") as f:
                    max_key = int(f.readline())
        else:
            if os.path.isfile(max_key_file):
                with open(max_key_file, "r") as f:
                    max_key = int(f.readline())
            else:
                max_key = int(max_key)

        if max_key_file is not None:
            self.key_file = open(max_key_file, "r")
        else:
            self.key_file = None

        self.key_dist = x_stress_util.Pareto(max_key)

    def update_max_key(self):
        if self.count % 1000 == 0:
            self.count = 1
            if self.key_file is not None:
                self.key_file.seek(0)
                new_max_key = x_stress_util.perform_ignore_interrupt(lambda: self.key_file.readline())
                self.key_dist = x_stress_util.Pareto(int(new_max_key))
        else:
            self.count += 1

    def run(self, conn):
        self.update_max_key()

        original_key = self.key_dist.get()
        key = md5.new(str(original_key)).hexdigest()
        rql_res = r.db(self.db).table(self.table).get(key).run(conn)

        if rql_res is None:
            return {"errors": ["key not found: %d" % original_key]}

        return {}
