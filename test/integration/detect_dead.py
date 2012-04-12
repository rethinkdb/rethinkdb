#!/usr/bin/env python
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import http_admin
from vcoptparse import *

op = OptParser()
op["log_file"] = StringFlag("--log-file", "stdout")
opts = op.parse(sys.argv)

c1 = http_admin.InternalCluster(log_file = opts["log_file"])
m1 = c1.add_machine()
m2 = c1.add_machine()
assert len(c1.get_directory()) == 2
c2 = c1.split([m2])
time.sleep(10)
assert len(c1.get_directory()) == 1
assert len(c1.get_directory()) == 1
c1.join(c2)
time.sleep(10)
assert len(c1.get_directory()) == 2
c1.shutdown()
