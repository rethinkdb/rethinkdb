# Copyright 2010-2012 RethinkDB, all rights reserved.
$LOAD_PATH.unshift('./lib')
load 'rethinkdb.rb'
include RethinkDB::Shortcuts
$port_offset = ENV['PORT_OFFSET'].to_i
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


time {
  (0...10).map {
    c = r.connect('localhost', $port_offset + 28015)
    (0...10).map {
      Thread.new {
        r.table('test').run(c)
      }
    }
  }.flatten.each {|t| t.join}
}
