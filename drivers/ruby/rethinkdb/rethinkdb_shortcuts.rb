load 'rethinkdb.rb'
module RethinkDB
  module Shortcuts_Mixin; def r; RethinkDB::RQL; end; end
end
