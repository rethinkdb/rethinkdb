load 'rethinkdb.rb'
module RethinkDB
  # Shortcuts to easily build RQL queries.
  module Shortcuts_Mixin
    # The only shortcut right now.  <b><tt>r.x</tt></b> will call the member
    # function <b>+x+</b> of Rethinkdb::RQL_Mixin.
    def r; RethinkDB::RQL; end
  end
end
