###
# Tests the driver cursor API
###

from os import getenv
from sys import path, argv
path.append("../../../drivers/python2")

import rethinkdb as r

c = r.connect(port=30363)

tbl = r.table('test')

num_rows = int(argv[1])
print "Testing for %d rows" % num_rows

cur = tbl.run(c)

i = 0
for row in cur:
    i += 1

if not i == num_rows:
    print "Test failed: expected %d rows but found %d." % (num_rows, i)
else:
    print "Test passed!"
