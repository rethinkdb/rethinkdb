#!/usr/bin/ruby
require 'pp'
x = ARGV.map {|f|
  File.open(f, "r").readlines.map{|l| eval l.chomp}
}.reduce(:+).sort_by{|x| x[1]}.each{|l|
  print l.inspect+"\n"
}

