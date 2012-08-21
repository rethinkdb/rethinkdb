load 'rethinkdb.rb'
# Right now, this is a place to record high-level spec changes that need to
# happen.  TODO:
# * UPDATE needs to be changed to do an implicit mapmerge on its righthand side
#   (this will make its behavior line up with the Python documentation).
# * UPDATE needs to work if you return NIL
# * MUTATE needs to be renamed to REPLACE (Joe and Tim and I just talked about
#   this) and maintain its current behavior rather than changing to match
#   UPDATE.
# * The following are unimplemented: REDUCE, GROUPEDMAPREDUCE, POINTMUTATE
# * The following are buggy: UPDATE, POINTUPDATE, FOREACH
# * I don't understand how GROUPEDMAPREDUCE works.
module RethinkDB
  # Shortcuts to easily build RQL queries.
  module Shortcuts_Mixin
    # The only shortcut right now.  <b><tt>r.x</tt></b> will call the member
    # function <b>+x+</b> of Rethinkdb::RQL_Mixin.
    def r; RethinkDB::RQL; end
  end
end
