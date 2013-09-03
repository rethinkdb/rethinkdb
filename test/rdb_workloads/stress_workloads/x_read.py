#!/usr/bin/env python
import md5, sys, os, errno, pareto

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', 'drivers', 'python')))
import rethinkdb as r

class Workload:
    def __init__(self, options):
        self.db = options["db"]
        self.table = options["table"]
        self.key_dist = pareto.Pareto(options["max_key"])
        self.count = 1

        if options["max_key_file"] is not None:
            self.key_file = open(options["max_key_file"], "r")
        else:
            self.key_file = None

    def update_max_key(self):
        if self.count % 1000 == 0:
            self.count = 1
            if self.key_file is not None:
                self.key_file.seek(0)
                new_max_key = self.key_file.read() # TODO: handle EINTR
                print "new_max_key: %s" % new_max_key
                self.key_dist = pareto.Pareto(int(new_max_key))
        else:
            self.count += 1

    def run(self, conn):
        self.update_max_key()

        original_key = self.key_dist.get()
        key = md5.new(str(original_key)).hexdigest()
        rql_res = r.db(self.db).table(self.table).get(key).run(conn)

        if rql_res is None:
            return { "errors": [ "key not found: %d" % original_key ] }

        return { }
