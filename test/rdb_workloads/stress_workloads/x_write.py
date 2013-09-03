#!/usr/bin/env python
import md5, sys, os, random, string, pareto
import multiprocessing

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', 'drivers', 'python')))
import rethinkdb as r

class Workload:
    def __init__(self, options):
        self.db = options["db"]
        self.table = options["table"]
        self.cid_dist = pareto.Pareto(1000)
        self.typ_dist = pareto.Pareto(10)

        # We use a multiprocessing shared value to store the latest key
        self.max_key = multiprocessing.Value('L', options["max_key"])

        if options["max_key_file"] is not None:
            self.key_file = open(options["max_key_file"], "w")
        else:
            self.key_file = None

    def update_max_key(self, new_key):
        if new_key % 100 == 0:
            if self.key_file is not None:
                self.key_file.seek(0)
                new_max_key = self.key_file.write(str(new_key)) # TODO: handle EINTR

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
