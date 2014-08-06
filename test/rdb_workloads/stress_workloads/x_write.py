#!/usr/bin/env python
import md5, sys, os, random, string, x_stress_util
import multiprocessing

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import utils
r = utils.import_python_driver()

class Workload:
    def __init__(self, options):
        self.db = options["db"]
        self.table = options["table"]
        self.cid_dist = x_stress_util.Pareto(1000)
        self.typ_dist = x_stress_util.Pareto(10)

        # Get the batch size to use
        self.batch_size = os.getenv("X_WRITE_BATCH_SIZE")
        if self.batch_size is None:
            self.batch_size = 1
        else:
            self.batch_size = int(self.batch_size)

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

        # We use a multiprocessing shared value to store the latest key
        self.max_key = multiprocessing.Value('L', max_key)

        if max_key_file is not None:
            self.key_file = open(max_key_file, "w")
            self.update_max_key(max_key)
        else:
            self.key_file = None

    def update_max_key(self, new_key):
        if new_key % 200 == 0:
            if self.key_file is not None:
                self.key_file.seek(0)
                new_max_key = x_stress_util.perform_ignore_interrupt(lambda: self.key_file.write("%d\n" % new_key))
                x_stress_util.perform_ignore_interrupt(lambda: self.key_file.flush())

    def get_key(self):
        self.max_key.acquire()
        try:
            new_key = self.max_key.value + 1
            self.max_key.value = new_key
        finally:
            self.max_key.release()
        self.update_max_key(new_key)
        return new_key

    def generate_nested(self, levels):
        nested = {}
        nested["foo"] = "".join(random.choice(string.letters + string.digits) for i in xrange(10))
        nested["bar"] = "".join(random.choice(string.letters + string.digits) for i in xrange(10))

        if levels > 0:
            nested["nested"] = self.generate_nested(levels - 1)
            nested["nested2"] = self.generate_nested(levels - 1)
            nested["nested3"] = self.generate_nested(levels - 1)

        return nested

    def generate_row(self):
        row = {}
        row["id"] = md5.new(str(self.get_key())).hexdigest()
        row["customer_id"] = "customer%03d" % self.cid_dist.get()
        row["type"] = "type%d" % self.typ_dist.get()
        row["datetime"] = r.now()
        row["nested"] = self.generate_nested(2)
        row["arr"] = [random.randint(0, 100000) for i in xrange(86)]
        row["arr2"] = ["".join(random.sample(string.letters + string.digits, 2)) for i in xrange(86)]
        row["flat"] = "".join(random.choice(string.letters + string.digits) for i in xrange(463))
        return row

    def run(self, conn):
        row_data = [self.generate_row() for i in xrange(self.batch_size)]
        rql_res = r.db(self.db).table(self.table).insert(row_data).run(conn)

        result = {}
        if rql_res["errors"] > 0:
            result["errors"] = [rql_res["first_error"]]

        return result
