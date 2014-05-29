#!/usr/local/bin/ruby
p "#{$0} starting..."

$LOAD_PATH.unshift('~/rethinkdb/drivers/ruby/lib')
load 'rethinkdb.rb'
include RethinkDB::Shortcuts

host = ARGV[0]
port = 28015 + ARGV[1].to_i
$start = Time.now
$timeout = (ENV['TIMEOUT'] || 1).to_i

def loop
  while Time.now < $start + $timeout
    begin
      yield
    rescue => e
      PP.pp e
    end
    sleep 0.1
  end
end

p "#{$0} Connecting to #{host}:#{port}..."
loop {
  r.connect(host: host, port: port).repl
  p "#{$0} Creating DB..."
  loop {
    r.db('test').info.run rescue r.db_create('test').run
    p "#{$0} Creating table..."
    loop {
      r.table('test').info.run rescue r.table_create('test').run
      p "#{$0} Populating..."
      loop {
        r.table('test').insert((0...100).map{{}}).run
        p "#{$0} DONE"
        exit 0
      }
    }
  }
}

p "#{$0} TIMED OUT"
exit 1
