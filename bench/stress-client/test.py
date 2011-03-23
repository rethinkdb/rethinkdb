#!/usr/bin/env python
import stress, os, time

client = stress.Client()
conn = stress.Connection("localhost:%d" % int(os.environ["RUN_PORT"]))
read_op = stress.ReadOp(client, conn, 1)
insert_op = stress.InsertOp(client, conn, 10)

def avg(x):
    if len(x) == 0: return "N/A"
    else: return sum(x) / len(x)

client.start()
for i in xrange(10):
    time.sleep(1)
    client.lock()
    read_lats = read_op.poll()["latency_samples"]
    insert_lats = insert_op.poll()["latency_samples"]
    print i, avg(insert_lats), avg(read_lats)
    read_op.reset()
    insert_op.reset()
    client.unlock()
client.stop()
