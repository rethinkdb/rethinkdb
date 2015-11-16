require 'pp'
require 'eventmachine'
require_relative './importRethinkDB.rb'

$port ||= (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i
ARGV.clear
$c = r.connect(port: $port).repl

r.table_create('test').run rescue nil
$tbl = r.table('test')
$tbl.index_create('a').run rescue nil
$tbl.index_create('m', multi: true).run rescue nil
$tbl.index_wait('a').run
$tbl.index_wait('m').run
$tbl.wait.run

$gen = Random.new
puts "Random seed: #{$gen.seed}"

$digits = 3
$rows = 10 ** $digits
def obj(i, prefix)
  s = sprintf("%0#{$digits}d", i)
  # 'id' varies between 10 and 100 characters so that the truncation
  # boundary moves all over the place.
  { 'id' => s + 'a'*$gen.rand(90) + (0...(10 - s.size)).map{|i| $gen.rand(10).to_s}.join,
    'a' => prefix + $gen.rand.to_s,
    'm' => [prefix + $gen.rand.to_s, prefix + s + $gen.rand.to_s, s].shuffle }
end

$prefixes = ['']
# 5 will never be truncated or go over the minimum truncation length.
# 80 will never be truncated but may go over the minimum truncation length.
# 200 will somtimes be truncated but will always go over the minimum truncation length.
# 300 will always be truncated and will always go over the minimum truncation length.
$prefixes += ['0', '1', '2'].flat_map{|n| [n*5, n*80, n*200, n*300]}

$data = (0...$rows).map{|i| obj(i, $prefixes[$gen.rand($prefixes.size)])}
def id_between(a, b)
  $data.bsearch {|x|
    return 1 if x['id'] < a
    return -1 if x['id'] >= b
    return 0
  }
end
$data_a = $data.sort{|a,b| a['a'] <=> b['a']}
$data_this_m = $data.flat_map{|x| x['m'].map{|y| x.merge({'this_m' => y})}}.sort {|a,b|
  a['this_m'] <=> b['this_m']
}
def clean_this_m arr
  arr.map{|x| x.dup.tap{|y| y.delete('this_m')}}
end
$data_m = clean_this_m($data_this_m)
$expected = {'id' => $data, 'a' => $data_a, 'm' => $data_m}

$tbl.delete.run
$tbl.insert($data).run

[1, 4].each {|shards|
  $shards = shards
  puts "Shards: #{shards}"
  $tbl.reconfigure({shards: shards, replicas: 2}).run
  $tbl.wait.run
  [{},
   {max_batch_rows: 1},
   {max_batch_rows: $gen.rand(100)},
   {max_batch_rows: 10**9, max_batch_bytes: 10**12, max_batch_seconds: 600}].each {|bo|
    $bo = bo
    puts "Batch options: #{bo}"

    ['id', 'a', 'm'].each {|f|
      puts "Ordering by '#{f}'..."
      $res = $tbl.orderby(index: f).run(bo).to_a
      raise "FAILED" if $res != $expected[f]

      puts "Ordering by r.desc('#{f}')..."
      $res = $tbl.orderby(index: r.desc(f)).run(bo).to_a
      raise "FAILED" if $res != $expected[f].reverse

      $start = $gen.rand($data.size)
      $end = $gen.rand($data.size)
      $start, $end = $end, $start if $start > $end
      if f == 'm'
        $left = $data_this_m[$start]['this_m']
        $right = $data_this_m[$end]['this_m']
        $ex = $data_this_m.select{|row| row['this_m'] >= $left && row['this_m'] < $right}
        $ex = clean_this_m($ex)
      else
        $left = $expected[f][$start][f]
        $right = $expected[f][$end][f]
        $ex = $expected[f].select{|row| row[f] >= $left && row[f] < $right}
      end
      puts "Between query: [#{$left}, #{$right})"
      puts "Between on '#{f}'..."
      $res = $tbl.between($left, $right, index: f).run(bo).to_a
      $set_res = $res.sort{|a,b| a['id'] <=> b['id']}
      $set_ex = $ex.sort{|a,b| a['id'] <=> b['id']}
      raise "FAILED" if $set_res != $set_ex

      puts "Between and Ordering by '#{f}'..."
      $res = $tbl.between($left, $right, index: f).orderby(index: f).run(bo).to_a
      raise "FAILED" if $res != $ex

      puts "Between and Ordering by r.desc('#{f}')..."
      $res = $tbl.between($left, $right, index: f).orderby(index: r.desc(f)).run(bo).to_a
      raise "FAILED" if $res != $ex.reverse
    }
  }
}

