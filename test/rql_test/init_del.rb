require 'eventmachine'
require_relative './importRethinkDB.rb'

$port ||= (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i
ARGV.clear
$c = r.connect(port: $port).repl

$seen_uninitial = false
class H < RethinkDB::Handler
  def initialize(field)
    @state = {}
    @field = field
  end
  def on_initial_val(val)
    id = val['id']
    raise "Duplicate initial val." if @state[id]
    @state[id] = val['a']
    p [:initial_val, val]
    r.table('test').get_all(10, index: 'a').update({a: 19.5}).run(noreply: true)
  end
  def on_uninitial_val(val)
    p [:uninitial_val, val]
    $seen_uninitial = true
    raise RuntimeError, "Bad uninitial val for #{val['id']}." if !@state[val['id']]
    @state.delete(val['id'])
  end
  def on_change(old_val, new_val)
    p [:change_val, old_val, new_val]
    if old_val == nil
      EM.defer {
        sleep 0.1
        $handle.close
        @state.each {|k,v|
          if k == 10
            raise RuntimeError, "#{k} = #{v}" if v != 19.5
          else
            raise RuntimeError, "#{k} = #{v}" if v != v
          end
        }
        EM.stop
      }
    end
  end
  def on_state(state)
    if state == "ready"
      r.table('test').insert({@field => 10.5, id: 10.5}).run
    end
  end
end

$statelog = []

r.table_create('test').run rescue nil
r.table('test').index_create('a').run rescue nil
r.table('test').index_wait('a').run
['a'].each {|field|
  r.table('test').delete.run
  r.table('test').insert((0...100).map{|i| {field => i, id: i, z: 9}}).run
  q = r.table('test').between(10, 20, index: field).changes(include_initial_vals: true)
  EM.run {
    $h = H.new(field)
    $handle = q.em_run($h, max_batch_rows: 1)
  }
}
raise RuntimeError, "No uninitial val." if !$seen_uninitial
