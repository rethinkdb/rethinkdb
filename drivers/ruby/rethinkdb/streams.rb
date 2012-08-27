module RethinkDB
  # A sequence of rows, e.g. what we get when reading from a table.
  class Stream_Expression < Expression
    # For each row in the invoking query, execute 1 or more write queries (to
    # execute more than 1, yield a list of write queries in the block).  For
    # example:
    #   table.foreach{|row| [table2.get(row[:id]).delete, table3.insert(row)]}
    # will, for each row in <b>+table+</b>, delete the row that shares that id
    # in <b>+table2+</b> and insert the row into <b>+table3+</b>.
    def foreach
      S.with_var { |vname,v|
        queries = yield(v)
        queries = [queries] if queries.class != Array
        queries.each{|q|
          if q.class != Write_Query
            raise TypeError, "Foreach requires query #{q.inspect} to be a write query."
          end}
        Write_Query.new [:foreach, @body, vname, queries]
      }
    end

    # Filter a stream, based on a predicate.  The provided block should take a
    # single variable, a row in the stream, and return either <b>+true+</b> if
    # it should be in the resulting stream of <b>+false+</b> otherwise.  If you
    # have a table <b>+table+</b>, the following are all equivalent:
    #   table.filter {|row| row[:id] < 5}
    #   table.filter {r[:id] < 5} # uses implicit variable
    # Alternately, you may provide an object as an argument, in which case the
    # <b>+filter+</b> will match JSON objects match the provided object's
    # attributes.  For example, if we have a table <b>+people+</b>, the
    # following are equivalent:
    #   people.filter{|row| row[:name].eq('Bob') & row[:age].eq(50)}
    #   people.filter({:name => 'Bob', :age => 50})
    # Note that the values of attributes may themselves be rdb queries.  For
    # instance, here is a query that matches anyone whose age is double their height:
    #   people.filter({:age => r.mul(:height, 2)})
    def filter(obj=nil)
      if obj
        raise SyntaxError,"Filter: Not a hash: #{obj.inspect}." if obj.class != Hash
        self.filter { |row|
          JSON_Expression.new [:call, [:all], obj.map{|kv|
                                 row.getattr(kv[0]).eq(S.r(kv[1]))}]
        }
      else
        S.with_var{|vname,v|
          self.class.new [:call, [:filter, vname, S.r(yield(v))], [@body]]}
      end
    end

    # Map a function over a stream, then concatenate the results together.  The
    # provided block should take a single variable, a row in the stream, and
    # return a list of rows to include in the resulting stream.  If you have a
    # table <b>+table+</b>, the following are all equivalent:
    #   table.concatmap {|row| [row[:id], row[:id]*2]}
    #   table.concatmap {[r[:id], r[:id]*2]} # uses implicit variable
    #   table.map{|row| [row[:id], row[:id]*2]}.reduce([]){|a,b| r.union(a,b)}
    def concatmap
      S.with_var { |vname,v|
        Stream_Expression.new [:call, [:concatmap, vname, S.r(yield(v))], [@body]]}
    end

    # Gets all rows of <b>+stream+</b> with keys between <b>+start_key+</b> and
    # <b>+end_key+</b> (inclusive).  You may also optionally specify the name of
    # the attribute to use as your key (<b>+keyname+</b>), but note that your
    # table must be indexed by that attribute.  Either <b>+start_key+</b> or
    # <b>+end_key+</b> may be nil, in which case that side of the range is
    # unbounded.  For example, if we have a table <b>+table+</b>, these are
    # equivalent:
    #   r.between(table, 3, 7)
    #   table.between(3,7)
    #   table.filter{|row| (row[:id] >= 3) & (row[:id] <= 7)}
    # as are these:
    #   table.between(nil,7,:index)
    #   table.filter{|row| row[:index] <= 7}
    def between(start_key, end_key, keyname=:id)
      opts = {:attrname => keyname}
      opts[:lowerbound] = (S.r start_key).sexp if start_key != nil
      opts[:upperbound] = (S.r end_key).sexp   if end_key   != nil
      self.class.new [:call, [:range, opts], [@body]]
    end

    # Map a function over a stream.  The provided block should take a single
    # variable, a row in the stream, and return a row in the resulting stream.
    # If you have a table <b>+table+</b>, the following are all equivalent:
    #   table.map {|row| row[:id]}
    #   table.map {r[:id]} # uses implicit variable
    def map
      S.with_var{|vname,v|
        Stream_Expression.new [:call, [:map, vname, S.r(yield(v))], [@body]]}
    end

    # Order <b>+stream+</b> by the attributes in <b>+orderings+</b>.  May also
    # be called as if it were an instance method of RQL_Query, for convenience.
    # For example, to sort first by name and then by social security number for
    # the table <b>+people+</b>, both of these work:
    #   r.orderby(people, :name, :ssn)
    #   people.orderby(:name, :ssn)
    # In place of an attribute name, you may provide a tuple of an attribute
    # name and a boolean specifying whether to sort in ascending order (which is
    # the default).  For example:
    #   people.orderby([:name, false], :ssn)
    # will sort first by name in descending order, and then by ssn in ascending
    # order.
    def orderby(*orderings)
      orderings.map!{|x| x.class == Array ? x : [x, true]}
      Stream_Expression.new [:call, [:orderby, *orderings], [@body]]
    end

    # Reduce a function over a stream.  Note that unlike Ruby's reduce, you
    # cannot omit the base case.  The block you privide should take two
    # arguments, just like Ruby's reduce.  For example, if we have a table
    # <b>+table+</b>, the following will add up the <b>+count+</b> attribute of
    # all the rows:
    #   table.map{|row| row[:count]}.reduce(0) {|a,b| a+b}
    # TODO: actual tests (reduce unimplemented right now)
    def reduce(base)
      S.with_var { |aname,a|
        S.with_var { |bname,b| JSON_Expression.new
          [:call,
           [:reduce, S.r(base), aname, bname, S.r(yield(a,b))],
           [@body]]}}
    end

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
