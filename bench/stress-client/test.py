#!/usr/bin/env python
import stress, os, time

client = stress.Client()
conn = stress.Connection("localhost:%d" % int(os.environ["RUN_PORT"]))
read_op = stress.ReadOp(client, conn, 1, batch_factor=1)
insert_op = stress.InsertOp(client, conn, 3)

client.start()
for i in xrange(10):
    time.sleep(1)
    client.lock()
    print i, read_op.poll()["queries"], insert_op.poll()["queries"]
    read_op.reset()
    insert_op.reset()
    client.unlock()
client.stop()
