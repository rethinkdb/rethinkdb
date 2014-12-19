p "setup.rb starting..."

$LOAD_PATH.unshift('~/rethinkdb/drivers/ruby/lib')
load 'rethinkdb.rb'
include RethinkDB::Shortcuts

host = ARGV[0]
port = 28015 + ARGV[1].to_i
$start = Time.now
$timeout = 5

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
  p "  Creating DB..."
  loop {
    r.db('test').info.run rescue r.db_create('test').run
    p "  Creating table..."
    loop {
      r.db('test').table('test').info.run rescue r.table_create('test').run
      p "  Populating..."
      loop {
        r.table('test').insert((0...100).map{{}}).run
        p "setup.rb DONE"
        exit 0
      }
    }
  }
}

p "setup.rb TIMED OUT"
exit 1
