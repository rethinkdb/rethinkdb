p "setup.rb starting..."

$LOAD_PATH.unshift('~/rethinkdb/drivers/ruby/lib')
load 'rethinkdb.rb'
include RethinkDB::Shortcuts

host = ARGV[0]
port = 28015 + ARGV[1].to_i
p "  Connecting to #{host}:#{port}..."
r.connect(host: host, port: port).repl


start = Time.now
timeout = 5

p "  Creating DB..."
while Time.now < start + timeout
  begin
    r.db('test').info.run rescue r.db_create('test').run
    p "  Creating table..."
    while Time.now < start + timeout
      begin
        r.table('test').info.run rescue r.table_create('test').run
        p "  Populating..."
        r.table('test').insert((0...1000).map{{}}).run

        p "setup.rb DONE"
        exit 0
      rescue RethinkDB::RqlError => e
        PP.pp e
      end
    end
  rescue RethinkDB::RqlError => e
    PP.pp e
  end
end

p "setup.rb TIMED OUT"
exit 1
