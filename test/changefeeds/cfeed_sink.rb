$LOAD_PATH.unshift('~/rethinkdb/drivers/ruby/lib')
load 'rethinkdb.rb'
include RethinkDB::Shortcuts

host = ARGV[0]
port = 28015 + ARGV[1].to_i
$start = Time.now
$timeout = 5

begin
  Timeout::timeout($timeout) {
    p "Sink at #{$$}..."
    r.connect(host: host, port: port).repl
    File.open("sink_#{$$}.t", "w") {|f|
      r.table('test').insert({pid: $$}).run
      r.table('test').changes.run.each {|change|
        raise `figlet FAILED` if (change['new_val']['pid'] rescue nil) == $$
        f.write(change.to_json + "\n")
        f.flush
      }
    }
  }
rescue Timeout::Error => e
  p "Sink at #{$$} DONE."
end
