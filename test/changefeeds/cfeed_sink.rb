$LOAD_PATH.unshift('~/rethinkdb/drivers/ruby/lib')
load 'rethinkdb.rb'
include RethinkDB::Shortcuts

host = ARGV[0]
port = 28015 + ARGV[1].to_i
$start = Time.now
$timeout = 60

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

p "Sink at #{$$}..."
r.connect(host: host, port: port).repl
File.open("#{$$}.t", "w") {|f|
  r.table('test').insert({pid: $$}).run
  r.table('test').changes.run.each{ |change|
    assert(change['new_val']['pid'] != $$)
    f.write(change.to_json)
    f.flush
  }
}
