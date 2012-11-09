# Copyright 2010-2012 RethinkDB, all rights reserved.
module RethinkDB
  # A lazy sequence of rows, e.g. what we get when reading from a table.
  # Includes the Sequence module, so look there for the methods.
  class Stream_Expression
    # Convert a stream into an array.  THINK CAREFULLY BEFORE DOING
    # THIS.  You can do more with an array than you can with a stream
    # (e.g., you can store an array in a variable), but that's because
    # arrays are stored in memory.  If your stream is big (e.g. you're
    # reading a giant table), that has serious performance
    # implications.  Also, if you return an array instead of a stream
    # from a query, the whole thing gets sent over the network at once
    # instead of lazily consuming chunks.  If you have a table <b>+table+</b> with at
    # least 3 elements, the following are equivalent:
    #   r[[1,1,1]]
    #   table.limit(3).map{1}.stream_to_array
    def stream_to_array(); JSON_Expression.new [:call, [:stream_to_array], [self]]; end
  end

  # A special case of Stream_Expression that you can write to.  You
  # will get a Multi_Row_Selection from most operations that access
  # tables.  For example, consider the following two queries:
  #   q1 = table.filter{|row| row[:id] < 5}
  #   q2 = table.map{|row| row[:id]}
  # The first query simply accesses some elements of a table, while the
  # second query does some work to produce a new stream.
  # Correspondingly, the first query returns a Multi_Row_Selection
  # while the second query returns a Stream_Expression.  So:
  #   q1.delete
  # is a legal query that will delete everything with <b>+id+</b> less
  # than 5 in <b>+table+</b>.  But:
  #   q2.delete
  # will raise an error.
  class Multi_Row_Selection < Stream_Expression
    attr_accessor :opts
    def initialize(body, context=nil, opts=nil) # :nodoc:
      super(body, context)
      if opts
        @opts = opts
      elsif @body[0] == :call and @body[2] and @body[2][0].kind_of? Multi_Row_Selection
        @opts = @body[2][0].opts
      end
    end

    def raise_if_outdated # :nodoc:
      if @opts and @opts[:use_outdated]
        raise RuntimeError, "Cannot write to outdated table."
      end
    end

    # Delete all of the selected rows.  For example, if we have
    # a table <b>+table+</b>:
    #   table.filter{|row| row[:id] < 5}.delete
    # will delete everything with <b>+id+</b> less than 5 in <b>+table+</b>.
    def delete
      raise_if_outdated
      Write_Query.new [:delete, self]
    end

    # Update all of the selected rows.  For example, if we have a table <b>+table+</b>:
    #   table.filter{|row| row[:id] < 5}.update{|row| {:score => row[:score]*2}}
    # will double the score of everything with <b>+id+</b> less than 5.
    # If the object returned in <b>+update+</b> has attributes
    # which are not present in the original row, those attributes will
    # still be added to the new row.
    #
    # If you want to do a non-atomic update, you should pass
    # <b>+:non_atomic+</b> as the optional variant:
    #   table.update(:non_atomic){|row| r.js("...")}
    # You need to do a non-atomic update when the block provided to update can't
    # be proved deterministic (e.g. if it contains javascript or reads from
    # another table).
    def update(variant=nil)
      raise_if_outdated
      S.with_var {|vname,v|
        Write_Query.new [:update, self, [vname, S.r(yield(v))]]
      }.apply_variant(variant)
    end

    # Replace all of the selected rows.  Unlike <b>+update+</b>, must return the
    # new row rather than an object containing attributes to be updated (may be
    # combined with RQL::merge to mimic <b>+update+</b>'s behavior).
    # May also return <b>+nil+</b> to delete the row.  For example, if we have a
    # table <b>+table+</b>, then:
    #   table.replace{|row| r.if(row[:id] < 5, nil, row)}
    # will delete everything with id less than 5, but leave the other rows untouched.
    #
    # If you want to do a non-atomic replace, you should pass
    # <b>+:non_atomic+</b> as the optional variant:
    #   table.replace(:non_atomic){|row| r.js("...")}
    # You need to do a non-atomic replace when the block provided to replace can't
    # be proved deterministic (e.g. if it contains javascript or reads from
    # another table).
    def replace(variant=nil)
      raise_if_outdated
      S.with_var {|vname,v|
        Write_Query.new [:replace, self, [vname, S.r(yield(v))]]
      }.apply_variant(variant)
    end
  end
end
