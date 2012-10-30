#!/usr/bin/env ruby
# Copyright 2010-2012 RethinkDB, all rights reserved.

$:.unshift "../../drivers/ruby/rethinkdb/"

host = ENV['HOST']
port = ENV['PORT'].to_i
raise RuntimeError,"Must specify HOST and PORT in environment." if
(!host || !port)

raise RuntimeError,"Expected at most one argument" if ARGV.length > 1
if ARGV.length == 1 then count = ARGV[0].to_i else count = 100 end

require 'rethinkdb.rb'
extend RethinkDB::Shortcuts_Mixin

$c = RethinkDB::Connection.new(host, port)

require 'net/http'
require 'uri'
require 'json'
uri = URI.parse("http://#{ENV['HOST']}:#{ENV['HTTP_PORT']}/ajax/semilattice/datacenters")
response = Net::HTTP.get_response(uri)
raise RuntimeError,"bad response: #{response}" if (response.code.to_i != 200)
datacenters_json = JSON.parse(response.body)
datacenter_name = datacenters_json.to_a[0][1]["name"]

require 'time'
i = 0
while i < count do
 tbl_name = rand().to_s
 start_time = Time.now
 r.db(ENV["DB_NAME"]).create_table(tbl_name, datacenter_name).run
 end_time = Time.now
 print "creating table \##{i+1} took #{end_time - start_time} seconds\n"
 i += 1
end