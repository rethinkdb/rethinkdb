#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

import os, httplib, sys, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import vcoptparse

# This isn't really a memcached workload...

op = vcoptparse.OptParser()
op['address'] = vcoptparse.StringFlag('--address', 'localhost:8080')
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
        fetch("/")
        time.sleep(1)
except KeyboardInterrupt:
    pass
