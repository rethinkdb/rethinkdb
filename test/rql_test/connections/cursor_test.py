from sys import argv
from random import randint
from subprocess import call
from sys import path, exit, stdout
path.insert(0, ".")
from test_util import RethinkDBTestServers

path.insert(0, "../../drivers/python")
import rethinkdb as r

server_build_dir = argv[1]
if len(argv) >= 3:
    lang = argv[2]
else:
    lang = None

with RethinkDBTestServers(4, server_build_dir=server_build_dir) as servers:
    port = servers.driver_port()
    c = r.connect(port=port)

    r.db('test').table_create('test').run(c)
    tbl = r.table('test')

    num_rows = randint(1111, 2222)

    print "Inserting %d rows" % num_rows
    documents = [{'id':i, 'nums':range(0, 500)} for i in xrange(0, num_rows)]
    chunks = (documents[i : i+100] for i in range(0, len(documents), 100))
    for chunk in chunks:
        tbl.insert(chunk).run(c)
        print '.',
        stdout.flush()
    print "Done\n"

    res = 0
    if not lang or lang == 'py':
        print "Running Python"
        res = res | call(["python", "connections/cursor.py", str(port), str(num_rows)])
        print ''
    if not lang or lang == 'js':
        print "Running JS"
        res = res | call(["node", "connections/cursor.js", str(port), str(num_rows)])
        print ''

    if res is not 0:
        exit(1)
