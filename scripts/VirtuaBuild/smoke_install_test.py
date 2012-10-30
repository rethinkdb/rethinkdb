# Copyright 2010-2012 RethinkDB, all rights reserved.
import time, sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, os.path.pardir, 'test', 'common')))
import workload_common

num_keys = 10000

class TestError(Exception):
    def __init__(self, str):
        self.str = str
    def __str__(self):
        return repr(self.str)

# simple test to make sure that an installation has basic functionality
def test_against(host, port):
    with workload_common.make_memcache_connection({"address": (host, port), "mclib": "pylibmc", "protocol": "text"}) as mc:
        while (mc.set("test", "test") == 0):
            time.sleep(5)

        for i in range(num_keys):
            if mc.set(str(i), str(i)) == 0:
                return False
        for i in range(num_keys):
            if mc.get(str(i)) != str(i):
                return False
        return True

# put some known data in the
def throw_migration_data(host, port):
    with workload_common.make_memcache_connection({"address": (host, port), "mclib": "pylibmc", "protocol": "text"}) as mc:
        while (mc.set("test", "test") == 0):
            time.sleep(5)

        for i in range(num_keys):
            if mc.set(str(i), str(i)) == 0:
                raise TestError("Write failed")

def check_migration_data(host, port):
    with workload_common.make_memcache_connection({"address": (host, port), "mclib": "pylibmc", "protocol": "text"}) as mc:
        while (mc.set("test", "test") == 0):
            time.sleep(5)

        for i in range(num_keys):
            if mc.get(str(i)) != str(i):
                raise TestError("Incorrect value")
