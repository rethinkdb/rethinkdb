#!/usr/bin/env python

import os
from sys import argv, path, exit, stdout
from random import randint
from subprocess import call
path.insert(0, os.path.join(os.path.dirname(__file__), os.pardir))
from test_util import RethinkDBTestServers

path.insert(0, "../../drivers/python")
import rethinkdb as r

server_build_dir = argv[1]
if len(argv) >= 3:
    lang = argv[2]
else:
    lang = None

res = 0

with RethinkDBTestServers(4, server_build_dir=server_build_dir) as servers:
    port = servers.driver_port()
    c = r.connect(port=port)
    r.db_create('test').run(c)
    r.db('test').table_create('test').run(c)
   
    basedir = os.path.dirname(__file__)
   
    stdout.flush()
    if not lang or lang == 'js':
        print "Running JS feeds"
        stdout.flush()
        res = res | call(["node", os.path.join(basedir, "feeds.js"), str(port)])
        print ''


if res is not 0:
    exit(1)
