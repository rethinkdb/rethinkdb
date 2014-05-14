p "wait_for_table.rb starting..."

$LOAD_PATH.unshift('~/rethinkdb/drivers/ruby/lib')
load 'rethinkdb.rb'
include RethinkDB::Shortcuts

host = ARGV[0]
port = 28015 + ARGV[1].to_i
$start = Time.now
$timeout = 10

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

p "  Connecting to #{host}:#{port}..."
loop {
  r.connect(host: host, port: port).repl
  p "Counting..."
  loop {
    r.table('test').count.run
    p "wait_for_table.rb DONE"
    exit 0
  }
}

p "wait_for_table.rb TIMED OUT"
exit 1
