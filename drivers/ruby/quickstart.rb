#!/usr/bin/env ruby
# Copyright 2010-2014 RethinkDB, all rights reserved.

expectedDriverPath = File.expand_path File.join(File.dirname(__FILE__), 'lib')

# -- load the driver

require File.join(expectedDriverPath, 'rethinkdb.rb')
include RethinkDB::Shortcuts

# -- test that we got the driver we expected

actualDriverPath = File.expand_path File.dirname(r.method(:connect).source_location[0])
if actualDriverPath != expectedDriverPath
  abort "Wrong Ruby driver loaded, expected from: " + expectedDriverPath + " but got: " + actualDriverPath
end

# --

$port_offset = ENV['PORT_OFFSET'].to_i
$c = r.connect(:host => 'localhost', :port => $port_offset + 28015, :db => 'test').repl
