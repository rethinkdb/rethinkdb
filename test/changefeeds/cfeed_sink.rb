#!/usr/local/bin/ruby
$LOAD_PATH.unshift('~/rethinkdb/drivers/ruby/lib')
load 'rethinkdb.rb'
include RethinkDB::Shortcuts

$port = ENV['PORT_OFFSET'].to_i + 28015 + ARGV[0].to_i
$tbl = ARGV[1]
$file = ARGV[2]

last_val = {}

begin
  Timeout::timeout(ENV['TIMEOUT'].to_i) {
    p "Sink at #{$file}..."
    r.connect(host: ENV['HOST'], port: $port).repl
    File.open($file, "w") {|f|
      r.table($tbl).insert({pid: $$}).run
      r.table($tbl).changes.run.each {|change|
        f.write(change.to_json + "\n")
        f.flush
        a = change['old_val']
        b = change['new_val']
        if (b && b['pid'] == $$)
          raise `figlet SAW_OWN_WRITE`
        end
        if a || b
          id = a ? a['id'] : b['id']
          if last_val[id] && a != last_val[id]
            raise `figlet UNORDERED`
          end
          last_val[id] = b
        end
      }
    }
  }
rescue Timeout::Error => e
  p "Sink at #{$file} DONE."
end
