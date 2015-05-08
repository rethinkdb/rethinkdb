require 'eventmachine'
require_relative './importRethinkDB.rb'

$port ||= (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i
ARGV.clear
$c = r.connect(port: $port).repl

class H < RethinkDB::Handler
  def initialize(field)
    @state = {}
    @field = field
  end
  def on_initial_val(val)
    id = val[@field]
    raise "Duplicate initial val." if @state[id]
    @state[id] = val['z']
    $statelog << @state.dup
    # PP.pp $statelog
    r.table('test').between([id-1, 11].max, 20, index: @field).update {|row|
      {z: row['z']+1}
    }.run(noreply: true)
    r.table('test').get_all(10, index: @field).update {|row|
      {z: row['z']+1}
    }.run(noreply: true)
    # PP.pp val
  end
  def on_change(old_val, new_val)
    # PP.pp [old_val, new_val]
    if !old_val
      EM.defer {
        sleep 0.1
        $handle.close
        @state.each {|k,v|
          if k == 10 || k == 19
            raise RuntimeError, "#{k} = #{v}" if v != 19
          else
            raise RuntimeError, "#{k} = #{v}" if v != k+1
          end
        }
        EM.stop
      }
    else
      id = old_val[@field]
      if @state[id] != old_val['z']
        raise RuntimeError, "Missed a change (#{old_val} -> #{new_val})."
      end
      @state[id] = new_val['z']
      $statelog << @state.dup
      # PP.pp $statelog
    end
  end
  def on_state(state)
    if state == "ready"
      r.table('test').insert({@field => 10.5}).run
    end
  end
end

$statelog = []

r.table_create('test').run rescue nil
r.table('test').index_create('a').run rescue nil
['id', 'a'].each {|field|
  r.table('test').delete.run
  r.table('test').insert((0...100).map{|i| {field => i, z: 9}}).run
  q = r.table('test').between(10, 20, index: field).changes(include_initial_vals: true)
  EM.run {
    $h = H.new(field)
    $handle = q.em_run($h, max_batch_rows: 1)
  }
}
