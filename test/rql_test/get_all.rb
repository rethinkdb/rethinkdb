require 'pp'
require 'eventmachine'
require_relative './importRethinkDB.rb'

$port ||= (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i
ARGV.clear
$c = r.connect(port: $port).repl

def to_truncated(i)
  'a'*300 + sprintf("%05d", i)
end

r.table_create('test').run rescue nil
# Note that for the simple table there's no oversharding because we
# use integer keys, and for the hard table there is because we use
# UUID keys.
r.table('test').reconfigure(replicas: 1, shards: 2).run
r.table('test').wait.run
indexes = ['a', 'val', 'truncated', 'collision', 'truncated_collision']
indexes.each {|index|
  r.table('test').index_create(index).run rescue nil
}

short_keys = [10, 20, -1, -1, 3, 3, 4, 3]
long_keys = (0...100).map{|i| (i-10)*2}*2

# Add these to the list if you have a lot of time to run the test:
# [{max_batch_rows: 1}, {max_batch_rows: 100}]
[{max_batch_rows: 10}, {}].each {|runopts|
  puts "----- Testing with runopts #{runopts} -----";

  puts "-- Setting up artificial table..."
  rows = (0...100).map{|i| {id: i}}
  res = r.db('rethinkdb').table('_debug_scratch').delete.run(runopts)
  res = r.db('rethinkdb').table('_debug_scratch').insert(rows).run(runopts)
  raise "Insert failed." if res['inserted'] != 100
  puts "Testing between..."
  res = r.db('rethinkdb').table('_debug_scratch').between(10, 50).run(runopts)
  raise "Between failed." if res.count != 40
  res = r.db('rethinkdb').table('_debug_scratch').
          order_by(index: 'id').between(10, 50).run(runopts)
  raise "Ordered between failed." if res.count != 40
  res = r.db('rethinkdb').table('_debug_scratch').
          order_by(index: r.desc('id')).between(10, 50).run(runopts)
  raise "Ordered between failed." if res.count != 40
  puts "Testing get_all..."
  res = r.db('rethinkdb').table('_debug_scratch').get_all(*short_keys).run(runopts)
  raise "Short get_all failed." if res.count != 6

  puts "-- Setting up simple table..."
  r.table('test').delete.run(runopts)
  res = r.table('test').insert((0...1000).map{|i| {id: i, a: i}}).run(runopts)
  raise "Insert failed." if res['inserted'] != 1000

  puts "Testing ordering..."
  res = r.table('test').run(runopts).to_a
  raise "Retrieve failed." if res.count != 1000
  res = r.table('test').order_by(index: 'id').run(runopts)
  raise "Ordered retrieve failed." if res.count != 1000
  res = r.table('test').order_by(index: r.desc('id')).run(runopts)
  raise "Descending retrieve failed." if res.count != 1000
  res = r.table('test').order_by(index: 'a').run(runopts)
  raise "Sindex retrieve failed." if res.count != 1000
  res = r.table('test').order_by(index: r.desc('a')).run(runopts)
  raise "Descending sindex retrieve failed." if res.count != 1000

  puts "Testing between..."
  res = r.table('test').between(200, 500).run(runopts).to_a
  raise "Retrieve failed." if res.count != 300
  res = r.table('test').order_by(index: 'id').between(200, 500).run(runopts)
  raise "Ordered retrieve failed." if res.count != 300
  res = r.table('test').order_by(index: r.desc('id')).between(200, 500).run(runopts)
  raise "Descending retrieve failed." if res.count != 300
  res = r.table('test').order_by(index: 'a').between(200, 500, index: 'a').run(runopts)
  raise "Sindex retrieve failed." if res.count != 300
  res = r.table('test').order_by(index: r.desc('a')).
          between(200, 500, index: 'a').run(runopts)
  raise "Descending sindex retrieve failed." if res.count != 300

  puts "Testing get_all..."
  res = r.table('test').get_all(*short_keys).run(runopts)
  raise "Short get_all failed." if res.to_a.count != 6
  res = r.table('test').get_all(*short_keys, index: 'a').run(runopts)
  raise "Short indexed get_all failed." if res.to_a.count != 6
  res = r.table('test').get_all(*long_keys).run(runopts)
  raise "Long get_all failed." if res.to_a.count != 180
  res = r.table('test').get_all(*long_keys, index: 'a').run(runopts)
  raise "Long indexed get_all failed." if res.to_a.count != 180

  ####

  puts "-- Setting up hard table..."
  r.table('test').delete.run(runopts)
  # TODO in the future:
  # * Multi-indexes (those follow the same code path but it would be
  #   good to test htem explicitly just for completeness).
  # * A mix of truncated and non-truncated keys in one index.
  input = (0...1000).map {|i|
    {
      val: i,
      truncated: to_truncated(i),
      collision: i % 10,
      truncated_collision: to_truncated(i % 10)
    }
  }
  r.table('test').insert(input).run(runopts)

  puts "Testing ordering..."
  res = r.table('test').run(runopts).to_a
  raise "Retrieve failed." if res.count != 1000
  ['id', 'val', 'truncated', 'collision', 'truncated_collision'].each {|field|
    puts "Testing #{field}..."
    res = r.table('test').order_by(index: field).run(runopts)
    raise "Ordered retrieve failed (#{field})." if res.count != 1000
    res = r.table('test').order_by(index: r.desc(field)).run(runopts)
    raise "Descending retrieve failed (#{field})." if res.count != 1000
  }

  puts "Testing between..."
  puts "Testing truncated..."
  res = r.table('test').order_by(index: 'truncated').
          between(to_truncated(200), to_truncated(500)).run(runopts)
  raise "Ordered retrieve failed (truncated)." if res.count != 300
  res = r.table('test').order_by(index: r.desc('truncated')).
          between(to_truncated(200), to_truncated(500)).run(runopts)
  raise "Descending retrieve failed (truncated)." if res.count != 300
  puts "Testing collision..."
  res = r.table('test').order_by(index: 'collision').between(2, 5).run(runopts)
  raise "Ordered retrieve failed (collision)." if res.count != 300
  res = r.table('test').order_by(index: r.desc('collision')).between(2, 5).run(runopts)
  raise "Descending retrieve failed (collision)." if res.count != 300
  puts "Testing truncated_collision..."
  res = r.table('test').order_by(index: 'truncated_collision').
          between(to_truncated(2), to_truncated(5)).run(runopts)
  raise "Ordered retrieve failed (truncated_collision)." if res.count != 300
  res = r.table('test').order_by(index: r.desc('truncated_collision')).
          between(to_truncated(2), to_truncated(5)).run(runopts)
  raise "Descending retrieve failed (truncated_collision)." if res.count != 300

  puts "Testing get_all..."
  res = r.table('test').get_all(*short_keys.map{|x| to_truncated(x)},
                                index: 'truncated').run(runopts)
  raise "Short truncated get_all failed." if res.to_a.count != 6
  res = r.table('test').get_all(*long_keys.map{|x| to_truncated(x)},
                                index: 'truncated').run(runopts)
  raise "Long truncated get_all failed." if res.to_a.count != 180
  def trans(x)
    legal = x.select{|x| x >= 0};
    illegal = x.select{|x| x < 0};
    legal.map{|x| x % 10} + illegal;
  end
  # We don't test long keys here because we have to scan the whole
  # table for these truncated keys so it would take forever.
  res = r.table('test').get_all(*trans(short_keys),
                                index: 'collision').run(runopts)
  raise "Short collision get_all failed." if res.to_a.count != 600
  res = r.table('test').get_all(*trans(short_keys).map{|x| to_truncated(x)},
                                index: 'truncated_collision').run(runopts)
  raise "Short truncated collision get_all failed." if res.to_a.count != 600

}
puts "Done!"
