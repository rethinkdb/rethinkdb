#!/usr/bin/ruby
# Copyright 2010-2012 RethinkDB, all rights reserved.
require 'pp'
x = ARGV.map {|f|
  File.open(f, "r").readlines.map{|l| eval l.chomp}
}.reduce(:+).sort_by{|x| x[1]}.each{|l|
  print l.inspect+"\n"
}

