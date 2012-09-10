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
    # instead of lazily consuming chunks.  Also has the shorter alias
    # <b>+to_array+</b>.  If you have a table <b>+table+</b> with at
    # least 3 elements, the following are equivalent:
    #   r[[1,1,1]]
    #   table.limit(3).map{1}.streamtoarray
    #   table.limit(3).map{1}.to_array
    def streamtoarray(); JSON_Expression.new [:call, [:streamtoarray], [@body]]; end
  end

  # A special case of Stream_Expression that you can write to.  You
  # will get a Multi_Row_selection from most operations that access
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
    # Delete all of the selected rows.  For example, if we have
    # a table <b>+table+</b>:
    #   table.filter{|row| row[:id] < 5}.delete
    # will delete everything with <b>+id+</b> less than 5 in <b>+table+</b>
    def delete
      Write_Query.new [:delete, @body]
    end

    # Update all of the selected rows.  For example, if we have a table <b>+table+</b>:
    #   table.filter{|row| row[:id] < 5}.update{|row| {:score => row[:score]*2}}
    # will double the score of everything with <b>+id+</b> less than 4.
    # If the object returned in <b>+update+</b> has attributes
    # which are not present in the original row, those attributes will
    # still be added to the new row.
    def update
      S.with_var{|vname,v| Write_Query.new [:update, @body, [vname, S.r(yield(v))]]}
    end

    # Replace all of the selected rows.  Unlike <b>+update+</b>, must return the
    # new row rather than an object containing attributes to be updated (may be
    # combined with RQL::mapmerge to mimic <b>+update+</b>'s behavior).
    # May also return <b>+nil+</b> to delete the row.  For example, if we have a
    # table <b>+table+</b>, then:
    #   table.mutate{|row| r.if(row[:id] < 5, nil, row)}
    # will delete everything with id less than 5, but leave the other rows untouched.
    def mutate
      S.with_var{|vname,v| Write_Query.new [:mutate, @body, [vname, S.r(yield(v))]]}
    end
  end
end
