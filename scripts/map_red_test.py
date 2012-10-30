# Copyright 2010-2012 RethinkDB, all rights reserved.
import riak
c = riak.RiakClient(port=2222)
foo_bucket = c.bucket('foo')
v1 = foo_bucket.new('fizz', data='a')
v1_link = foo_bucket.new('fizz_link_to', data='a\'')
v1_link.store()
v1.add_link(v1_link)
v1.store()

v2 = foo_bucket.new('buzz', data='b')
v2_link = foo_bucket.new("buzz_link_t", data='b\'')
v2_link.store()
v2.add_link(v2_link)
v2.store()

q = riak.RiakMapReduce(c)
q.add_object(v1)
q.add_object(v2)
#q.link(keep=False)
q.map("function(v) {return [v];}")
q.reduce("function(v) { return v; }")
print q.run()
