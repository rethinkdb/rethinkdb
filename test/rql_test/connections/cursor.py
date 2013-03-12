###
# Tests the driver cursor API
###

from os import getenv
from sys import path, argv
path.append("../../drivers/python")

import rethinkdb as r

num_rows = int(argv[2])
print "Testing for %d rows" % num_rows
port = int(argv[1])

c = r.connect(port=port)

tbl = r.table('test')

cur = tbl.run(c)
if not type(cur) is r.Cursor
    print "Test failed -- r.Cursor name is wrong"

i = 0
for row in cur:
    i += 1

if not i == num_rows:
    print "Test failed: expected %d rows but found %d." % (num_rows, i)
else:
    print "Test passed!"
