#!/usr/local/bin/ruby
$LOAD_PATH.unshift('~/rethinkdb/drivers/ruby/lib')
load 'rethinkdb.rb'
include RethinkDB::Shortcuts

$port = ENV['PORT_OFFSET'].to_i + 28015 + ARGV[0].to_i
$tbl = ARGV[1]
$file = ARGV[2]

begin
  Timeout::timeout(ENV['TIMEOUT'].to_i) {
    p "Sink at #{$$}..."
    r.connect(host: ENV['HOST'], port: $port).repl
    File.open($file, "w") {|f|
      r.table($tbl).insert({pid: $$}).run
      r.table($tbl).changes.run.each {|change|
        raise `figlet FAILED` if (change['new_val']['pid'] rescue nil) == $$
        f.write(change.to_json + "\n")
        f.flush
      }
    }
  }
rescue Timeout::Error => e
  p "Sink at #{$file} DONE."
end
