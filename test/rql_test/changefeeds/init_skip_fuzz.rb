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

$nrows = 1000
$init_pop = (0..$nrows).map {|i|
  {'id' => i, 'a' => i}
}
$pop = $init_pop.dup
PP.pp $tbl.insert($pop).run
$tbl.reconfigure({shards: 2, replicas: 1}).run
$tbl.wait(:wait_for=>"all_replicas_ready").run

def assert(x, *args)
  raise RuntimeError, "Assert failed (#{args})." if !x
  x
end

$log = Hash[(0..$nrows).map{|i| [i, []]}]
$canon_log = Hash[(0..$nrows).map{|i| [i, []]}]

class Printer < RethinkDB::Handler
  def on_initial_val(val)
    if val # This can go away once #5153 is fixed
      printf("i")
      $log[val['id']] << [:init, val]
      if val['id'] == $nrows
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
      $log[a['id']] << [:change, a, b]
      assert($set[a['id']] == a, a, b)
      $set[a['id']] = b
    else
      $log[b['id']] << [:change, a, b]
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

$copts = {include_initial: true, include_states: true, squash: true}
['a', 'id'].each {|index|
  $index = index
  [{}, {max_batch_rows: 10}, {max_batch_rows: 1}].each {|ropts|
    $ropts = ropts
    puts "\nRUN: #{$index} #{$ropts}"
    $seen_end = false
    $set = {}
    EM.run {
      $handler = $tbl.between(r.minval, r.maxval, index: $index) \
        .order_by(index: r.desc($index)) \
        .map{|x| x.merge({'foo' => 'bar'})}
        .changes($copts).em_run(Printer, $ropts)
      $sleep_dur = 0.005
      EM.defer {
        while !$seen_end
          printf("X")
          index = $gen.rand($nrows)
          val = $gen.rand
          $wopts = {noreply: true}
          $tbl.get(index).update({'c' => -val}).run($wopts)
          $tbl.get(index).update({'d' => -val}).run($wopts)
          $tbl.get(index).update({'c' => val}).run($wopts)
          $tbl.get(index).update({'d' => val}).run($wopts)
          sleep $sleep_dur
          $sleep_dur *= 1.001
          new_val = $pop[index].merge({'c' => val, 'd' => val})
          $canon_log[index] << [:change, $pop[index], new_val]
          $pop[index] = new_val
        end
      }
    }
  }
}
"done"
