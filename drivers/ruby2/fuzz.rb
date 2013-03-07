#!/usr/bin/ruby
# Copyright 2010-2012 RethinkDB, all rights reserved.
load 'quickstart2.rb'

include RethinkDB

$port_base = 32600
$port_base ||= ARGV[0].to_i # 0 if none given

class FuzzConn < Connection
  def send packet
    super send_fuzz(packet)
  end
  def dispatch msg
    super dispatch_fuzz(msg)
  end
end

$c = FuzzConn.new('localhost', $port_base + 28015).repl
