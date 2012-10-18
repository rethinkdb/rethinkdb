#!/usr/bin/ruby
require 'pp'
require 'socket'
require 'optparse'
$LOAD_PATH.unshift('../rethinkdb')
require 'rethinkdb.rb'
include RethinkDB::Shortcuts

File.open(ARGV[0], "r") {|f|
  f.each {|l|
    peer,time,packet =  eval(l.chomp)
    begin
      protob = Query.new.parse_from_string(packet[4..-1])
      PP.pp [peer, time, protob]
    rescue Exception => e
      print "UNPARSABLE: #{l}"
    end
  }
}
