require 'pp'
require 'eventmachine'
require_relative '../importRethinkDB.rb'

$port ||= (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i
ARGV.clear
$c = r.connect(port: $port).repl

$n_shards = rand(5)
def n_shards()
  $n_shards = ($n_shards + 1) % 5
  puts "Resharding to  #{$n_shards + 1} shards."
  return $n_shards + 1;
end

r.table_create('test').run rescue nil
r.table('test').index_create('a').run rescue nil
r.table('test').reconfigure({shards: n_shards(), replicas: 1}).run
r.table('test').wait.run
r.table('test').delete.run
r.table('test').insert((0...1000).map{|i| {id: i, a: i}}).run

[{max_batch_rows: 1}, {max_batch_rows: 10}].each {|bo|
  [lambda {|x| x},
   lambda {|x| x.get_all(r.args(r.range(100).coerce_to('array')))},
   lambda {|x| x.get_all(r.args(r.range(100).coerce_to('array')), index: 'a')},
   lambda {|x| x.orderby(index: 'id')},
   lambda {|x| x.orderby(index: r.desc('id'))},
   lambda {|x| x.orderby(index: 'a')},
   lambda {|x| x.orderby(index: r.desc('a'))}].each {|func|
    op = func.call(r.table('test'))
    puts "Testing #{op.inspect}.run(#{bo})..."
    stream = op.run(bo)
    x = stream.next
    # puts "res_size: #{stream.instance_eval("@results").size}"
    r.table('test').reconfigure(shards: n_shards(), replicas: 1).run
    r.table('test').wait.run
    y = nil
    begin
      y = stream.to_a.size
    rescue RethinkDB::RqlError => e
      puts "Stream successfully aborted: #{e}."
    end
    raise "Stream failed to abort (got #{y})!" if y
  }
}

[{max_batch_rows: 1}, {max_batch_rows: 10}].each {|bo|
  [lambda {|x| x},
   lambda {|x| x.get_all(r.args(r.range(100).coerce_to('array')))},
   lambda {|x| x.get_all(r.args(r.range(100).coerce_to('array')), index: 'a')},
   lambda {|x| x.between(100, 800, index: 'id')},
   lambda {|x| x.between(100, 800, index: 'a')}].each {|func|
    op = func.call(r.table('test'))
    puts "Testing #{op.inspect}.run(#{bo})..."
    stream1 = op.changes(include_initial: true).run(bo)
    x = stream1.next
    stream2 = op.changes().run(bo)
    r.table('test').reconfigure(shards: n_shards(), replicas: 1).run
    r.table('test').wait.run
    y = nil
    begin
      puts "Testing #{op.inspect}.changes(include_initial: true).run(#{bo})..."
      loop { y = stream1.next(3) }

    rescue RethinkDB::RqlError => e
      puts "Stream successfully aborted: #{e}."
      y = nil
      begin
        puts "Testing #{op.inspect}.changes().run(#{bo})..."
        loop { y = stream2.next(3) }
      rescue RethinkDB::RqlError => e
        puts "Stream successfully aborted: #{e}."
        y = nil
      end
    end
    raise "Stream failed to abort (got #{y})!" if y
  }
}
