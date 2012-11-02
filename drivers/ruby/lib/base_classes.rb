# Copyright 2010-2012 RethinkDB, all rights reserved.
#TODO: toplevel doc
module RethinkDB
  class RQL_Query; end
  class Read_Query < RQL_Query # :nodoc:
  end
  class Write_Query < RQL_Query # :nodoc:
  end
  class Meta_Query < RQL_Query # :nodoc:
  end

  module Sequence; end
  class JSON_Expression < Read_Query; include Sequence; end
  class Single_Row_Selection < JSON_Expression; end
  class Stream_Expression < Read_Query; include Sequence; end
  class Multi_Row_Selection < Stream_Expression; end

  class Database; end
  class Table; end

  class Connection; end
  class Query_Results; include Enumerable; end

  module RQL; end

  # Shortcuts to easily build RQL queries.
  module Shortcuts
    # The only shortcut right now.  May be used as a wrapper to create
    # RQL values or as a shortcut to access function in the RQL
    # namespace.  For example, the following are equivalent:
    #   r.add(1, 2)
    #   RethinkDB::RQL.add(1, 2)
    #   r(1) + r(2)
    def r(*args)
      (args == [] ) ? RethinkDB::RQL : RQL.expr(*args)
    end
  end
end
