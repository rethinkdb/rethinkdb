#!/usr/bin/ruby
$LOAD_PATH.unshift('./rethinkdb')
require 'pp'
require 'socket'
require 'optparse'
require 'rethinkdb.rb'

$opt = {}
OptionParser.new{|opts|
  opts.banner = "Usage: table_prune.rb [options]"

  $opt[:host] = "localhost"
  opts.on('-h', '--host HOST', 'Fuzz on host HOST (default localhost)') {|h|
    $opt[:host] = h
  }

  $opt[:port] = "0"
  opts.on('-p', '--port PORT', 'Prune on PORT+28015 (default 0)') {|p|
    $opt[:port] = p
  }

  $opt[:limit] = "30"
  opts.on('-l', '--limit LIMIT', 'Limit the number of tables to LIMIT (default 30)') {|l|
    $opt[:limit] = l
  }
}.parse!

include RethinkDB::Shortcuts
$c = RethinkDB::Connection.new($opt[:host], $opt[:port].to_i + 28015)
while true
  dbs = r.list_dbs.run
  tables = dbs.map{|x| [x, r.db(x).list_tables.run]}
  tables = tables.reject{|db, tbls| tbls.length == 0}
  num_tables = tables.map{|db, tbls| tbls.length}.reduce(:+)
  print "number of tables: #{num_tables}\n"
  if num_tables > $opt[:limit].to_i
    db_del, tables_del = tables[0]
    r.db(db_del).drop_table(tables_del[0]).run
  else
    sleep 10
  end
end
