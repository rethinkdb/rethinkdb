#!/usr/bin/env ruby
# Copyright 2015-2016 RethinkDB, all rights reserved.

require_relative './importRethinkDB.rb'

$port ||= (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i
ARGV.clear
$c = r.connect(port: $port).repl

dbName = 'test'
tableName = File.basename(__FILE__).gsub('.', '_')

r.expr([dbName]).set_difference(r.db_list()).for_each{|row| r.db_create(row)}.run
r.expr([tableName]).set_difference(r.db(dbName).table_list()).for_each{|row| r.db(dbName).table_create(row)}.run
$tbl = r.db(dbName).table(tableName)

$tbl.wait(:wait_for=>"all_replicas_ready").run
$tbl.index_create('a').run rescue nil
$tbl.index_wait.run

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
