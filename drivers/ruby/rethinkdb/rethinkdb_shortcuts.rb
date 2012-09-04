require 'rethinkdb.rb'
# Right now, this is a place to record high-level spec changes that need to
# happen.  TODO:
# * MUTATE needs to be renamed to REPLACE (Joe and Tim and I just talked about
#   this) and maintain its current behavior rather than changing to match
#   UPDATE.
# * The following are unimplemented: REDUCE, GROUPEDMAPREDUCE, POINTMUTATE
# * The following are buggy: FOREACH
# * I don't understand how GROUPEDMAPREDUCE works.
module RethinkDB
  # Shortcuts to easily build RQL queries.
  module Shortcuts_Mixin
    # The only shortcut right now.  <b><tt>r.x</tt></b> will call the member
    # function <b>+x+</b> of Rethinkdb::RQL_Mixin.
    def r; RethinkDB::RQL; end
  end
end
