#!/usr/bin/env python

from __future__ import print_function

import os
from sys import argv, path, exit, stdout
from random import randint
from subprocess import call
path.insert(0, os.path.join(os.path.dirname(__file__), os.pardir))
from test_util import RethinkDBTestServers

path.insert(0, "../../drivers/python")
import rethinkdb as r

try:
    xrange
except NameError:
    xrange = range

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
    tbl = r.table('test')

    num_rows = randint(1111, 2222)

    print("Inserting %d rows" % num_rows)
    range500 = list(range(0, 500))
    documents = [{'id':i, 'nums':range500} for i in xrange(0, num_rows)]
    chunks = (documents[i : i + 100] for i in xrange(0, len(documents), 100))
    for chunk in chunks:
        tbl.insert(chunk).run(c)
        print('.', end=' ')
        stdout.flush()
    print("Done\n")
    
    basedir = os.path.dirname(__file__)
    
    if not lang or lang == 'py':
        print("Running Python")
        res = res | call([os.environ.get('INTERPRETER_PATH', 'python'), os.path.join(basedir, "cursor.py"), str(port), str(num_rows)])
        print('')
    if not lang or lang == 'js':
        print("Running JS")
        res = res | call(["node", os.path.join(basedir, "cursor.js"), str(port), str(num_rows)])
        print('')
    if not lang or lang == 'js-promise':
        print("Running JS Promise")
        res = res | call(["node", os.path.join(basedir, "promise.js"), str(port), str(num_rows)])
        print('')


if res is not 0:
    exit(1)
