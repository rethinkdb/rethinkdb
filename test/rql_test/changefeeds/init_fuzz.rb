require 'eventmachine'
require_relative '../importRethinkDB.rb'

$port ||= (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i
ARGV.clear
$c = r.connect(port: $port).repl

$gen = Random.new
puts "Random seed: #{$gen.seed}"

$tbl = r.table('test')
r.table_drop('test').run rescue nil
r.table_create('test').run
$tbl.index_create('a').run
$tbl.index_wait.run

$init_pop = (0..25000).map {|i|
  {'id' => i, 'a' => i % 20, 'bloat' => '#'*250}
}
$pop = $init_pop.dup
PP.pp $tbl.insert($pop).run
$tbl.reconfigure({shards: 2, replicas: 1}).run
$tbl.wait().run

def assert(x, *args)
  raise RuntimeError, "Assert failed (#{args})." if !x
  x
end

class Printer < RethinkDB::Handler
  def on_initial_val(val)
    if val # This can go away once #5153 is fixed
      printf("i")
      if val['id'] == 25000
        EM.defer {
          sleep 1
          $seen_end = true
        }
      end
      $set[val['id']] = val
    end
  end
  def on_uninitial_val(val)
    printf("u")
  end
  def on_change(a, b)
    printf("c")
    if a
      assert($set[a['id']] == a)
      $set[a['id']] = b
    else
      assert(b)
      $set[b['id']] = b
    end
  end
  def on_state(x)
    printf(" #{x} ")
    if x == 'ready'
      EM.defer {
        sleep 3
        $handler.close
        EM.stop
      }
    end
  end
  def on_error(err)
    PP.pp err
  end
end

[true, false].each {|squash|
  $copts = {include_initial: true, include_states: true, squash: squash}
  [{}, {max_batch_rows: 10}, {max_batch_rows: 1}].each {|ropts|
    $ropts = ropts
    $seen_end = false
    $set = {}
    EM.run {
      $handler = $tbl.get_all(0, index: 'a').changes($copts).em_run(Printer, $ropts)
      # $tbl.between(475, 525, index: 'a').changes($opts).em_run(Printer)
      EM.defer {
        while !$seen_end
          printf("X")
          index = $gen.rand(25000)
          $tbl.get(index).update({'a' => -1}).run(noreply: true)
          $pop[index] = $pop[index].merge({'a' => -1})
          sleep 0.05
        end
      }
    }

    $pop.each_index {|i|
      if $pop[i]['a'] == 0
        assert($set[i] == $pop[i], i)
      else
        assert(!$set[i], i)
      end
    }
  }
}
"done"
