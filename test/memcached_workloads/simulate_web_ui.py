# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/python
import sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import memcached_workload_common
import httplib, time

# This isn't really a memcached workload...

op = memcached_workload_common.option_parser_for_socket()
opts = op.parse(sys.argv)
host, port = opts["address"]

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