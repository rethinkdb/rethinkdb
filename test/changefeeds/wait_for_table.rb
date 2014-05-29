#!/usr/local/bin/ruby
$LOAD_PATH.unshift('~/rethinkdb/drivers/ruby/lib')
load 'rethinkdb.rb'
include RethinkDB::Shortcuts

$port = ENV['PORT_OFFSET'].to_i + 28015 + ARGV[0].to_i
$tbl = ARGV[1]
$start = Time.now
$timeout = ENV['TIMEOUT'].to_i

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

p "#{$0} Connecting to #{ENV['HOST']}:#{$port}..."
loop {
  r.connect(host: ENV['HOST'], port: $port).repl
  p "  Counting..."
  loop {
    r.table($tbl).count.run
    p "#{$0} DONE"
    exit 0
  }
}

p "#{$0} TIMED OUT"
exit 1
