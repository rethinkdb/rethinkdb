#!/usr/bin/env python
import md5, sys, os, random, time, errno, string
from pareto import Pareto

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', 'drivers', 'python')))
import rethinkdb as r

class Workload:
    def __init__(self, options):
        self.db = options["db"]
        self.table = options["table"]
        self.cid_dist = Pareto(1000)
        self.typ_dist = Pareto(10)

        # Load the max_key
        # We use a named pipe "./db.table.key_pipe" to coordinate max key
        new_fifo = False
        fifo_filename = "./%s.%s.key_pipe" % (self.db, self.table)
        try:
            os.mkfifo(fifo_filename)
            new_fifo = True
        except OSError as ex:
            if ex.errno != errno.EEXIST:
                raise

        self.key_pipe = os.open(fifo_filename, os.O_RDWR)
        if new_fifo:
            self.write_key(options["max_key"])

    # This sort of depends on being atomic - which means partial reads are a problem
    # Thus we write and read exactly 32 bytes every time
    def get_key(self):
        key_str = os.read(self.key_pipe, 32)
        if len(key_str) != 32:
            raise RuntimeError("Incomplete read from key_pipe: %d of 32" % len(key_str))
        key = int(key_str) + 1
        self.write_key(key)
        return key

    def write_key(self, key):
        os.write(self.key_pipe, "%032d" % key)

    def generate_nested(self, levels):
        nested = { }
        nested["foo"] = "".join(random.choice(string.letters + string.digits) for i in xrange(10))
        nested["bar"] = "".join(random.choice(string.letters + string.digits) for i in xrange(10))

        if levels > 0:
            nested["nested"] = self.generate_nested(levels - 1)
            nested["nested2"] = self.generate_nested(levels - 1)
            nested["nested3"] = self.generate_nested(levels - 1)

    def generate_row(self):
        row = { }
        row["id"] = md5.new(str(self.get_key())).hexdigest()
        row["customer_id"] = "customer%03d" % self.cid_dist.get()
        row["type"] = "type%d" % self.typ_dist.get()
        row["datetime"] = r.now()
        row["nested"] = self.generate_nested(2)
        row["arr"] = [ random.randint(0, 100000) for i in xrange(86) ]
        row["arr2"] = [ random.sample(string.letters + string.digits, 2) for i in xrange(86) ]
        row["flat"] = "".join(random.choice(string.letters + string.digits) for i in xrange(463))
        return row

    def run(self, conn):
        rql_res = r.db(self.db).table(self.table).insert(self.generate_row()).run(conn)

        result = { }
        if rql_res["errors"] > 0:
            result["errors"] = [ rql_res["first_error"] ]

        return result
