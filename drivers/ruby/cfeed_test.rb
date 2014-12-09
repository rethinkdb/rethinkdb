$LOAD_PATH.unshift('./lib'); load 'rethinkdb.rb'; include RethinkDB::Shortcuts
require 'timeout'

module RethinkDB
  class RQL
    def run_safe
      ret = run
      PP.pp ret
      raise "Error: #{ret.inspect}" if (ret['errors'] != 0 rescue nil)
    end
  end
end

$port_offset = 32700
$c = r.connect(:host => 'localhost', :port => $port_offset + 28015,
               :db => 'test').repl.auto_reconnect

def frac2uuid frac
  val = ("%030x" % (0xffffffffffffffffffffffffffffffff * frac).to_i).chars
  "00000000-0000-0000-0000-000000000000".chars.map{|c|
    c == '-' ? c : val.shift
  }.join
end
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
$shards = {unsharded: 1, sharded: $machines, oversharded: $machines*2}
$limit_sizes = [1, 10]
$max_pop_size = 1024

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
        deleted = @t[k][v].delete(obj)
        raise "Failed to delete." if !deleted
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
  def in_state?(rows)
    @t['id'].map{|x| x[0]}.sort != rows.sort
  end
  def assert_state(rows)
    raise "Incorrect state." if !in_state?(rows)
  end
  def sync(query)
    rows = query.current_val
    while !in_state?(rows)
      change = query.wait_for_change
      del(change['old_val'])
      add(change['new_val'])
    end
  end
end

class Query
  attr_reader :size
  def initialize(pop_size, limit_size, query)
    @pop_size = pop_size
    @limit_size = limit_size
    @init_size = [pop_size, limit_size].min

    @query = query
    @changes = query.changes.run_safe
  end
  def current_val
    @query.run_safe
  end
  def init
    @changes.take(@init_size).map{|row| row['new_val']}
  end
  def wait_for_change(tm=5)
    Timeout::timeout(tm) {
      change = @changes.take(1)
      raise "Unexpected init change." if change['new_val'] == change['old_val']
      return change
    }
  end
end

$shards.each {|name, nshards|
  # TODO: shard here
  $limit_sizes.each {|limit_size|
    $pop_sizes = [0, limit_size - 1, limit_size, $max_pop_size].uniq
    $pop_sizes.each {|pop_size|
      begin
        sub_pop = (0...pop_size).map{|i| pop(i, pop_size)}
        $t.insert(sub_pop).run_safe

        $queries = ['id', 'a', 'b', 'c', 'd', 'm'].flatmap{|field|
          [$t.orderby(index: field).limit(limit_size),
           $t.orderby(index: r.desc(field)).limit(limit_size)]
        }
        ['id', 'a', 'b', 'c', 'd', 'm'].each{|field|
          forward = $t.orderby(index: field)
          backward = $t.orderby(index: r.desc(field))
          [forward, backward].each{|dir|
            orig_state = q.current_val
            first = dir[0].default(nil).run_safe
            dir.limit(1).delete.run_safe; em.sync(q)
            if first; $t.insert(first).run_safe; em.sync(q); end
            assert_state(orig_state)
          }
        }
      ensure
        $t.delete.run_safe
      end
    }
  }
}

frac2uuid 0.23444567
