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
      PP.pp [e, e.backtrace]
    end
    sleep 0.1
  end
end

p "Source at #{$$}..."
r.connect(host: host, port: port).repl
docs = (0...1000).map{|i| {n: i, pid: $$, target: "#{host}:#{port}"}}
loop {
  res = r.table('test').insert(docs, durability:'soft').run
  r.table('test').filter{|row| row['pid'].eq($$) & (row['n'] % 2).eq(1)}.delete.run
  r.table('test').filter{|row| row['pid'].eq($$) & (row['n'] % 2).eq(0)}.update {|row|
    { target: row['target'] + " UPDATED", updated: true }
  }.run
}
p "Source at #{$$} DONE."
