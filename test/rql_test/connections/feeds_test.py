#!/usr/bin/env python

from __future__ import print_function

import os, sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), os.pardir))
import test_util

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, os.path.pardir, 'common')))
import utils

r = utils.import_python_driver()

server_build_dir = sys.argv[1]
if len(sys.argv) >= 3:
    lang = sys.argv[2]
else:
    lang = None

res = 0

with test_util.RethinkDBTestServers(4, server_build_dir=server_build_dir) as servers:
    port = servers.driver_port()
    c = r.connect(port=port)
    r.db_create('test').run(c)
    r.db('test').table_create('test').run(c)
   
    basedir = os.path.dirname(__file__)
   
    sys.stdout.flush()
    if not lang or lang == 'js':
        print("Running JS feeds")
        sys.stdout.flush()
        res = res | subprocess.call(["node", os.path.join(basedir, "feeds.js"), str(port)])
        print('')

if res is not 0:
    sys.exit(1)
