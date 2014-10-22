#!/usr/local/bin/ruby
# Copyright 2014 RethinkDB, all rights reserved.

# -- get the location of the driver

rethinkdbDriverPath = File.expand_path(ENV['RUBY_DRIVER_DIR'] || File.join(File.dirname(__FILE__), '..', '..', 'build', 'drivers', 'ruby', 'lib'))

# -- import the RethinkDB driver

if !Dir.exists?(rethinkdbDriverPath) or !File.exists?(File.join(rethinkdbDriverPath, 'rethinkdb.rb'))
  abort "Unable to locate the Ruby driver from " + rethinkdbDriverPath
end
if !File.exists?(File.join(rethinkdbDriverPath, 'ql2.pb.rb'))
  abort "Ruby driver is not built: " + rethinkdbDriverPath
end

require File.join(rethinkdbDriverPath, 'rethinkdb.rb')
include RethinkDB::Shortcuts

# -- validate the location

if rethinkdbDriverPath != File.dirname(r.method(:connect).source_location[0])
  abort "Wrong Ruby driver loaded, expected from: " + rethinkdbDriverPath + " but got :" + File.dirname(r.method(:connect).source_location[0])
end
