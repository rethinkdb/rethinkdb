require 'ripl'
$LOAD_PATH.unshift('./lib')
load 'rethinkdb.rb'
include RethinkDB::Shortcuts

module Cli
  def self.start
    port_offset = ENV['PORT_OFFSET'].to_i
    $c = r.connect(:host => 'localhost', :port => port_offset + 28015, :db => 'test').repl
    Ripl::Runner.run
  end
end
