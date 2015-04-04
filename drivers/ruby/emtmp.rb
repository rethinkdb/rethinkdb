load 'quickstart3.rb'
require 'eventmachine'

$c.reconnect
$conn = $c

# RSI: we appear to be discarding changes
# multi-indexes
# union
# artificial feeds
# old val with no new val (sindexes)


class H < RethinkDB::Handler
  def initialize(field)
    @state = {}
    @field = field
  end
  def on_initial_val(val)
    id = val[@field]
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
        EM.stop
        @state.each {|k,v|
          if k == 10 || k == 19
            raise RuntimeError, "#{k} = #{v}" if v != 19
          else
            raise RuntimeError, "#{k} = #{v}" if v != k+1
          end
        }
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

['id', 'a'].each {|field|
  r.table('test').delete.run
  r.table('test').insert((0...100).map{|i| {field => i, z: 9}}).run
  q = r.table('test').between(10, 20, index: field).changes(return_initial: true)
  EM.run {
    $h = H.new(field)
    $handle = q.em_run($h, max_batch_rows: 1)
  }
}
