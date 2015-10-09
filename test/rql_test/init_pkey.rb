require 'eventmachine'
require_relative './importRethinkDB.rb'

$port ||= (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i
ARGV.clear
$c = r.connect(port: $port).repl

class H < RethinkDB::Handler
  def initialize
    @state = {}
  end
  def on_initial_val(val)
    id = val['id']
    raise "Duplicate initial val." if @state[id]
    @state[id] = val['z']
    $statelog << @state.dup
    r.table('test').between([id-1, 11].max, 20).update {|row|
      {z: row['z']+1}
    }.run(noreply: true)
    r.table('test').get(10).update {|row|
      {z: row['z']+1}
    }.run(noreply: true)
  end
  def on_change(old_val, new_val)
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
      id = old_val['id']
      if @state[id] != old_val['z']
        raise RuntimeError, "Missed a change (#{old_val} -> #{new_val})."
      end
      @state[id] = new_val['z']
      $statelog << @state.dup
    end
  end
  def on_state(state)
    if state == "ready"
      r.table('test').insert({id: 10.5}).run
    end
  end
end

r.table_create('test').run rescue nil
(0...100).each {|i|
  p i
  $statelog = []
  r.table('test').delete.run
  r.table('test').insert((0...100).map{|i| {id: i, z: 9}}).run
  q = r.table('test').between(10, 20).changes(include_initial: true)
  EM.run {
    $h = H.new
    $handle = q.em_run($h, max_batch_rows: 1)
  }
}
