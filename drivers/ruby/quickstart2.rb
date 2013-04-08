# Copyright 2010-2012 RethinkDB, all rights reserved.
$LOAD_PATH.unshift('./lib')
load 'rethinkdb.rb'
include RethinkDB::Shortcuts
$port_offset = 14850
$c = r.connect('localhost', $port_offset + 28015, 'test').repl
$rdb = r.db('test').table('test')

def time(f = PP.method(:pp))
  t1 = Time.now
  begin
    f.call(yield)
  ensure
    t2 = Time.now
    return t2 - t1
  end
end
