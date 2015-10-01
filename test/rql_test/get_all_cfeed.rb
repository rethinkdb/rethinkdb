require 'pp'
require 'eventmachine'
require_relative './importRethinkDB.rb'

$port ||= (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i
ARGV.clear
$c = r.connect(port: $port).repl

r.table_create('test').run rescue nil
r.table('test').wait.run
r.table('test').index_create('a').run rescue nil

$stop_id = rand
$short_keys = [10, 20, -1, -1, 3, -2, 3, -2, 4, 3, $stop_id]
$long_keys = (0...100).map{|i| (i-10)*2}*2 + [$stop_id]

class Handler < RethinkDB::Handler
  def initialize(max_active, set)
    @state = []
    @active = 0
    @max_active = max_active
    @stop_id = rand
    @set = set
  end
  def on_open(caller)
    @active += 1
    if @active == @max_active
      r.table('test').update({b: rand}).run
      r.table('test').insert({id: -1, a: -1}).run
      r.table('test').between(0, 5).update({b: rand}).run
      r.table('test').get(-1).delete.run
      r.table('test').insert({id: $stop_id, a: $stop_id}).run
    end
  end
  def on_close
    @active -= 1
    if @active == 0
      keys_seen = []
      @state.each {|x|
        if x[0] == :change
          id = x[2]['id'] rescue x[3]['id']
          keys_seen << id
        else
          raise "Unexpected error: #{x}"
        end
      }
      keys_seen.sort!
      keys_desired = ((@set.reject{|x| x < -1 || x >= 1000 || x == $stop_id} +
                       @set.reject{|x| x < -1 || x >= 5 || x == $stop_id})*2).sort

      if keys_seen != keys_desired
        # We set globals here to make debugging the broken test easier
        # if you're running this in an interpreter.
        $keys_seen = keys_seen
        $keys_desired = keys_desired
        $state = @state
        raise "Wrong keys seen (got #{keys_seen} but expected #{keys_desired})."
      end
      EM.stop
    end
  end
  def on_error(err, caller)
    @state << [:err, caller.object_id, err]
  end
  def on_change(old_val, new_val, caller)
    id = new_val['id'] rescue old_val['id']
    if id == $stop_id
      caller.close
    else
      @state << [:change, caller.object_id, old_val, new_val]
    end
  end
end

# Add these to the list if you have a lot of time to run the test:
# [{max_batch_rows: 1}, {max_batch_rows: 100}]
[{max_batch_rows: 10}, {}].each {|runopts|
  puts "----- Testing with runopts #{runopts} -----";

  puts "Setting up table..."
  r.table('test').delete.run
  r.table('test').insert((0...1000).map{|i| {id: i, a: i}}).run
  # We reconfigure after inserting so that we actually put data on both shards.
  r.table('test').reconfigure(replicas: 1, shards: 2).run
  r.table('test').wait.run

  puts "Testing changefeeds on short keys..."
  EM.run {
    h = Handler.new(2, $short_keys)
    r.table('test').get_all(*$short_keys).changes.em_run(h)
    r.table('test').get_all(*$short_keys, index: 'a').changes.em_run(h)
  }
  r.table('test').get($stop_id).delete.run

  puts "Testing changefeeds on long keys..."
  EM.run {
    h = Handler.new(2, $long_keys)
    r.table('test').get_all(*$long_keys).changes.em_run(h)
    r.table('test').get_all(*$long_keys, index: 'a').changes.em_run(h)
  }
}
puts "Done!"
