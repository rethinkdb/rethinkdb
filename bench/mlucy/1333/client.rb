#!/usr/bin/ruby
require 'rethinkdb'
include RethinkDB::Shortcuts

poc = ARGV[0].split(":")
puts "Connecting to #{poc}..."
r.connect(:host => poc[0], :port => poc[1].to_i).repl
tstart = Time.now
r.db('savr').table('log_populate').get_all('tom@ngonews.com', :index => 'user_id').limit(50).orderby('timestamp').count.run
tend = Time.now
p(tend - tstart)
