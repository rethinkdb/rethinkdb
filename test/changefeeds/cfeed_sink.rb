#!/usr/local/bin/ruby
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
      PP.pp [e, e.backtrace]
    end
    sleep 0.1
  end
end

begin
  Timeout::timeout($timeout) {
    p "Sink at #{$$}..."
    r.connect(host: host, port: port).repl
    File.open(ARGV[2] || "sink_#{$$}.t", "w") {|f|
      r.table('test').insert({pid: $$}).run
      $success = true
      r.table('test').changes.run.each {|change|
        raise `figlet FAILED` if (change['new_val']['pid'] rescue nil) == $$
        f.write(change.to_json + "\n")
        f.flush
      }
    }
  }
rescue Timeout::Error => e
  exit 1 if !$success
  p "Sink at #{$$} DONE."
end
