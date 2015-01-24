$LOAD_PATH.unshift('./lib'); load 'rethinkdb.rb'; include RethinkDB::Shortcuts
require 'timeout'

module RethinkDB
  class RQL
    def run_safe
      ret = run
      # PP.pp ret
      raise "Error: #{ret.inspect}" if (ret['errors'] && ret['errors'] != 0 rescue nil)
      ret
    end
  end
end

$port_offset = 32700
$c = r.connect(:host => 'localhost', :port => $port_offset + 28015,
               :db => 'test').repl.auto_reconnect
# RSI: %032x
# RSI: keys of different lengths
# Make sure to keep the "\x01"s in there, they test some key unmangling logic.
def frac2uuid frac
  val = ("%030x" % (0xffffffffffffffffffffffffffffffff * frac).to_i).chars
  "00000000-00\x0100-00\x0200-0000-000000000000".chars.map{|c|
    c != '0' ? c : val.shift
  }.join
end
# RSI: add truncated keys to `m`?
# RSI: some pkeys should contain \1 and \2 to test unmangling logic.
def pop(i, max)
  { 'id' => frac2uuid((i*1.0)/max),
    'a' => i,
    'b' => i % 2,
    'c' => "a"*(i.even? ? 10 : 1000) + ("%020d" % i),
    'd' => "a" * 1000,
    'm' => [i, i, i.to_s, i % 2] }
end

# TODO: better assert
def assert(arg)
  raise "Assertion failed." if !arg
end

$tname = 'test'
r.table_create($tname).run rescue nil
$t = r.table($tname)
$t.index_create('a').run rescue nil
$t.index_create('b').run rescue nil
$t.index_create('c').run rescue nil
$t.index_create('d').run rescue nil
$t.index_create('m', multi: true).run rescue nil
$t.delete.run_safe
#

$machines = 2
$shards = {oversharded: $machines*2, sharded: $machines, unsharded: 1}
$limit_sizes = [512, 1, 10]
$max_pop_size = 1024

$synclog = []

class Emulator
  def initialize(objs)
    @t = Hash.new{|h,k| h[k] = Hash.new{|h,k| h[k] = []}}
    objs.each{|obj| add(obj)}
  end
  def add(obj)
    obj.each {|k, vs|
      raise "All keys should be strings." if !k.is_a?(String)
      [*vs].each {|v|
        @t[k][v] << obj
      }
    }
  end
  def del(obj)
    obj.each {|k, vs|
      [*vs].each {|v|
        # We don't use normal `delete` because we only want to delete 1.
        idx = @t[k][v].index(obj)
        raise "Failed to delete." if !idx
        @t[k][v].delete_at(idx)
      }
    }
  end
  def del_id(id)
    del(@t['id'][id])
  end
  def get_all(**opts)
    raise "Too many optargs." if opts.size != 1
    opts.map{|k, v| @t[k.to_s][v]}[0]
  end
  def size(field)
    @t[field.to_s].size
  end
  def state
    @t['id'].flat_map{|x| x[1]}.map{|x| x.to_a.sort}.sort
  end
  def in_state?(rows)
    state == rows
  end
  def assert_state(raw_rows)
    rows = raw_rows.map{|x| x.to_a.sort}.sort
    $assert_state = [state, rows]
    raise "Incorrect state." if !in_state?(rows)
  end
  def sync(query)
    rows = query.current_val.map{|x| x.to_a.sort}.sort
    while !in_state?(rows)
      $synclog << [:sync, state.sort, rows.sort]
      change = query.wait_for_change
      del(change['old_val']) if change['old_val']
      add(change['new_val']) if change['new_val']
    end
  end
end

class Query
  attr_reader :size
  def initialize(raw_pop_size, limit_size, query)
    @raw_pop_size = raw_pop_size
    if query.inspect.match('"m"')
      @pop_size = @raw_pop_size * 4
    else
      @pop_size = @raw_pop_size
    end
    @limit_size = limit_size

    @init_size = [@pop_size, limit_size].min

    @query = query
    @changes = query.changes.run_safe.each
  end
  def current_val
    @query.run_safe.to_a
  end
  def init
    (0...@init_size).map{@changes.next['new_val']}
  end
  def wait_for_change(tm=5)
    Timeout::timeout(tm) {
      change = @changes.next
      $bad_change = change
      raise "Bad change." if change['new_val'] == change['old_val']
      return change
    }
  end
end

$fields = ['id', 'a', 'b', 'c', 'd', 'm']

# RSI: batch size
$shards.each {|name, nshards|
  PP.pp [:nshards, nshards]
  $t.reconfigure(shards: nshards, replicas: 1)
  # RSI: shard here
  $limit_sizes.each_with_index {|limit_size, limit_size_index|
    PP.pp [:limit_size, limit_size]
    pop_sizes = [0, limit_size - 1, limit_size, $max_pop_size].uniq
    pop_sizes.each_with_index {|pop_size, pop_size_index|
      PP.pp [:pop_size, pop_size]
      begin
        sub_pop = (0...pop_size).map{|i| pop(i, pop_size)}
        $t.insert(sub_pop).run_safe

        queries = $fields.flat_map {|field|
          [$t.orderby(index: field).limit(limit_size),
           $t.orderby(index: r.desc(field)).limit(limit_size)]
        }
        queries.each_with_index {|raw_query, query_index|
          # next if query_index < 1
          $rqlast = raw_query
          $qlast = q = Query.new(pop_size, limit_size, raw_query)
          em = Emulator.new(q.init)
          PP.pp [:query, q]
          $fields.each_with_index {|field, field_index|
            PP.pp [:field, field]
            forward = $t.orderby(index: field)
            backward = $t.orderby(index: r.desc(field))
            [forward, backward].each {|dir|
              PP.pp [:dir, dir]
              orig_state = q.current_val
              # PP.pp [:orig_state, orig_state]
              first = dir[0].default(nil).run_safe
              # PP.pp [:first, first]
              PP.pp [:deleting, dir.limit(1).run_safe]
              dir.limit(1).delete.run_safe; em.sync(q)
              PP.pp [:inserting, first]
              if first; $t.insert(first).run_safe; em.sync(q); end
              PP.pp [:asserting]
              em.assert_state(orig_state)
              PP.pp [:asserted]
            }
            PP.pp [[limit_size_index, pop_size_index, query_index, field_index],
                   [$limit_sizes.size, pop_sizes.size, queries.size, $fields.size]]
          }
        }
        $t.delete.run_safe
      ensure
        # $t.delete.run_safe
      end
    }
  }
}
