# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/env python
import stress, os, time

client = stress.Client()
conn = stress.Connection("localhost:%d" % int(os.environ["RUN_PORT"]))
key_generator = stress.SeedKeyGenerator()
model = stress.ConsecutiveSeedModel()
read_op = stress.ReadOpGenerator(1, key_generator, model.live_chooser(), conn)
client.add_op(1, read_op)
insert_op = stress.InsertOpGenerator(1, key_generator, model.insert_chooser(), model, conn)
client.add_op(1, insert_op)

def avg(x):
    if len(x) == 0: return "N/A"
    else: return sum(x) / len(x)

client.start()
for i in xrange(10):
    time.sleep(1)
    read_op.lock()
    insert_op.lock()
    read_lats = read_op.poll()["latency_samples"]
    insert_lats = insert_op.poll()["latency_samples"]
    print i, avg(insert_lats), avg(read_lats)
    read_op.reset()
    insert_op.reset()
    read_op.unlock()
    insert_op.unlock()
client.stop()
