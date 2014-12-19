#!/usr/bin/env python

from __future__ import print_function

import os, sys

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, os.path.pardir, 'common')))
import utils

r = utils.import_python_driver()

executable_path = sys.argv[1] if len(sys.argv) > 1 else utils.find_rethinkdb_executable()
os.environ['RDB_EXE_PATH'] = executable_path

with driver.Cluster.create_cluster(initial_servers=4, executable_path=executable_path) as cluster:
    port = list(cluster.processes)[0].driver_port
    
    c = r.connect(port=port)
    r.db_create('test').run(c)
    r.db('test').table_create('test').run(c)
   
    sys.stdout.flush()
    if not lang or lang == 'js':
        print("Running JS feeds")
        sys.stdout.flush()
        sys.exit(subprocess.call(["node", os.path.join(os.path.dirname(__file__), "feeds.js"), str(port)]))
