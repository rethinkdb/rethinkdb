#!/usr/bin/env python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import httplib, time
from vcoptparse import OptParser, StringFlag

# This isn't really a memcached workload...

op = OptParser()
op['address'] = StringFlag('--address', 'localhost:8080')
opts = op.parse(sys.argv)
host, port = opts["address"].split(':')

def fetch(resource, expect = [200]):
    conn = httplib.HTTPConnection(host, port)
    conn.request("GET", resource)
    response = conn.getresponse()
    assert response.status in expect

fetch("/")

try:
    while True:
        fetch("/ajax")
        fetch("/ajax/progress")
        fetch("/ajax/log/_?max_length=10&min_timestamp=0")
        for i in xrange(5):
            fetch("/ajax/stat")
            time.sleep(1)
except KeyboardInterrupt:
    pass
