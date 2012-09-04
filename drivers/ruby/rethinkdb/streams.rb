module RethinkDB
  # A sequence of rows, e.g. what we get when reading from a table.
  class Stream_Expression < Expression
    # Convert from a stream to an array.  Also has the synonym <b>+to_array+</b>.
    # While most sequence functions are polymorphic and handle
    # both arrays and streams, when arrays or streams need to be combined
    # (e.g. via <b>+union+</b>) you need to explicitly convert between the
    # types.  This is mostly for safety, but also because which type you're
    # working with effects error handling.  The following are equivalent:
    #   table.streamtoarray
    #   table.to_array
    def streamtoarray(); JSON_Expression.new [:call, [:streamtoarray], [@body]]; end
  end

  # A special case of Stream_Expression that you can write to.
  class Multi_Row_Selection < Stream_Expression
    # Delete all rows of the invoking query.  For example, if we have
    # a table
    # <b>+table+</b>:
    #   table.filter{|row| row[:id] < 5}.delete
    # will construct a query that deletes everything with <b>+id+</b> less than
    # 5 in <b>+table+</b>
    def delete
      Write_Query.new [:delete, @body]
    end

    # Update all rows of the invoking query.  For example, if we have a table
    # <b>+table+</b>:
    #   table.filter{|row| row[:id] < 5}.update{|row| {:score => row[:score]*2}}
    # will construct a query that doubles the score of everything with
    # <b>+id+</b> less tahn 4.  If the object returned in <b>+update+</b>
    # has attributes which are not present in the original row, those attributes
    # will still be added to the new row.
    # TODO: doesn't work
    def update
      S.with_var{|vname,v| Write_Query.new [:update, @body, [vname, S.r(yield(v))]]}
    end

    # Replace all rows of the invoking query.  Unlike update, must return the
    # new row rather than an object containing attributes to be updated (may be
    # combined with <b>+mapmerge+</b> to achieve a similar effect to update).
    # May also return <b>+nil+</b> to delete the row.  For example, if we have a
    # table <b>+table+</b>, then:
    #   table.mutate{|row| if(row[:id] < 5, nil, row)}
    # will delete everything with id less than 5, but leave the other rows untouched.
    def mutate
      S.with_var{|vname,v| Write_Query.new [:mutate, @body, [vname, S.r(yield(v))]]}
    end
  end
end
