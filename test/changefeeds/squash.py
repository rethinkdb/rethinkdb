#!/usr/bin/env python
# Copyright 2014-2015 RethinkDB, all rights reserved.

import itertools, squash_base

def TestFactory(name, squash, field, generator, records, limit, multi=False, base=squash_base.SquashBase):
    def setUp(self, **kwargs):
        base.setUp(self, squash, field, generator, records, limit, multi)

    return type(name, (base, ), {"setUp": setUp})

# ===== main

if __name__ == '__main__':
    import unittest

    for squash_name, squash in {
            "True": True,
            "Two": 2.0
        }.iteritems():
        for index_name, (field, generator, multi) in {
                "Id": ("id", itertools.count(), False),
                "Secondary": ("secondary", itertools.count(), False),
                "Multi": ("multi", squash_base.MultiGenerator(), True)
            }.iteritems():
            for records_limit_name, (records, limit) in {
                    "Empty": (0, 10),
                    "HalfFull": (5, 10),
                    "SingleHashShard": (10, 2),
                    "MultipleHashShards": (20, 2)
                }.iteritems():
                name = "Squash%s%s%s" % (squash_name, index_name, records_limit_name)
                globals()[name] = TestFactory(
                    name, squash, field, generator, records, limit, multi)

    unittest.main()
