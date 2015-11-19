require 'pp'
require 'eventmachine'
require_relative './importRethinkDB.rb'

$port ||= (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i
ARGV.clear
$c = r.connect(port: $port).repl

r.table_create('test').run rescue nil
r.table('test').index_create('a').run rescue nil
r.table('test').wait.run
$tbl = r.table('test')
$ids = (0...100).to_a
$scales = [10, 50, 100]
(0...10).each {|i|
  puts "Run: #{i}:"
  $scales.each {|scale|
    puts "Scale #{scale}:"
    $samp = $ids.sample(rand(scale))
    puts "Inserting #{$samp}..."
    $tbl.delete.run
    $tbl.insert($samp.map{|i| {id: i, a: i}}).run
    [[], $samp.sample($samp.size / 2)].each {|subpop|
      $gets = $ids.sample(rand(scale)+1) + subpop
      [{}, {max_batch_rows: 1}, {max_batch_rows: 5}].each {|bo|
        $bo = bo
        ['id', 'a'].each {|field|
          $field = field
          puts "Getting (#{$field} #{$bo}) #{$gets}..."
          $res = $tbl.get_all(*$gets, index: $field).run(bo).to_a.map{|x| x['id']}.sort
          $expected = $gets.select{|x| $samp.include?(x)}.sort
          raise "#{$res} != #{$expected}" if $res != $expected
        }
      }
    }
  }
}
