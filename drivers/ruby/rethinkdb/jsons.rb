module RethinkDB
  # A query returning a JSON expression.  Most of the functions that you
  # can run on a JSON object are found in RethinkDB::RQL and accessed
  # with the <b>+r+</b> shortcut.  For convenience, may of the
  # functions in Rethinkdb::RQL can be run as if they were instance
  # methods of JSON_Expression.  For example, the following are
  # equivalent:
  #   r.add(1, 2)
  #   r[1].add(2)
  #   r[3]
  #
  # Running a JSON_Expression query will return a literal Ruby
  # representation of the resulting JSON, rather than a stream or a
  # string.  For example, the following are equivalent:
  #   r.add(1,2).run
  #   3
  # As are:
  #   r[{:a => 1, :b => 2}].pickattrs(:a).run
  #   {'a' => 1}
  # (Note that the symbol keys were coerced into string keys in the
  # object.  JSON doesn't distinguish between keys and strings.)
  class JSON_Expression
    # If <b>+ind+</b> is a symbol or a string, gets the corresponding
    # attribute of an object.  For example, the following are equivalent:
    #   r[{:id => 1}][:id]
    #   r[{:id => 1}]['id']
    #   r[1]
    # Otherwise, if <b>+ind+</b> is a number or a range, invokes RQL::[]
    def [](ind)
      if ind.class == Symbol || ind.class == String
        B.alt_inspect(JSON_Expression.new [:call, [:getattr, ind], [self]]) {
          "#{self.inspect}[#{ind.inspect}]"
        }
      else
        super
      end
    end

    # Append a single element to an array.  The following are equivalent:
    #   r[[1,2,3,4]]
    #   r[[1,2,3]].append(4)
    def append(el)
      JSON_Expression.new [:call, [:arrayappend], [self, S.r(el)]]
    end

    # Get an attribute of a JSON object.  The following are equivalent:
    #   r[{:id => 1}].getattr(:id)
    #   r[1]
    def getattr(attrname)
      JSON_Expression.new [:call, [:getattr, attrname], [self]]
    end

    # Check whether a JSON object has a particular attribute.  The
    # following are equivalent:
    #   r[{:id => 1}].hasattr(:id)
    #   r[true]
    def hasattr(attrname)
      JSON_Expression.new [:call, [:hasattr, attrname], [self]]
    end

    # Construct a JSON object that has a subset of the attributes of
    # another JSON object by specifying which to keep.  The following are equivalent:
    #   r[{:a => 1, :b => 2, :c => 3}].pickattrs(:a, :c)
    #   r[{:a => 1, :c => 3}]
    def pickattrs(*attrnames)
      JSON_Expression.new [:call, [:pickattrs, *attrnames], [self]]
    end

    # Construct a JSON object that has a subset of the attributes of
    # another JSON object by specifying which to drop.  The following are equivalent:
    #   r[{:a => 1, :b => 2, :c => 3}].without(:a, :c)
    #   r[{:b => 2}]
    def without(*attrnames)
      JSON_Expression.new [:call, [:without, *attrnames], [self]]
    end

    # Convert from an array to a stream.  Also has the synonym
    # <b>+to_stream+</b>.  While most sequence functions are polymorphic
    # and handle both arrays and streams, when arrays or streams need to be
    # combined (e.g. via <b>+union+</b>) you need to explicitly convert between
    # the types.  This is mostly for safety, but also because which type you're
    # working with effects error handling.  The following are equivalent:
    #   r[[1,2,3]].arraytostream
    #   r[[1,2,3]].to_stream
    def arraytostream(); Stream_Expression.new [:call, [:arraytostream], [self]]; end

    # Prefix numeric -.  The following are equivalent:
    #   -r[1]
    #   r[-1]
    def -@; JSON_Expression.new [:call, [:subtract], [self]]; end
    # Prefix numeric +.  The following are equivalent:
    #   +r[-1]
    #   r[-1]
    def +@; JSON_Expression.new [:call, [:add], [self]]; end
  end

  # A query representing a variable.  Produced by e.g. RQL::var.  This
  # is its own class because it needs to behave correctly when spliced
  # into a string (see RQL::javascript).
  class Var_Expression < JSON_Expression
    # Convert from an RQL query representing a variable to the name of that
    # variable.  Used e.g. in constructing javascript functions.
    def to_s
      if @body.class == Array and @body[0] == :var
      then @body[1]
      else raise TypeError, 'Can only call to_s on RQL_Queries representing variables.'
      end
    end
  end

  # Like Multi_Row_Selection, but for a single row.  E.g.:
  #   table.get(0)
  # yields a Single_Row_Selection
  class Single_Row_Selection < JSON_Expression
    # Analagous to Multi_Row_Selection#update
    def update(*args, &b)
      b = S.arg_or_block(*args, &b)
      S.with_var {|vname,v|
        Write_Query.new [:pointupdate, *(@body[1..-1] + [[vname, S.r(b.call(v))]])]}
    end

    # Analagous to Multi_Row_Selection#mutate
    def mutate(*args, &b)
      b = S.arg_or_block(*args, &b)
      S.with_var {|vname,v|
        Write_Query.new [:pointmutate, *(@body[1..-1] + [[vname, S.r(b.call(v))]])]}
    end

    # Analagous to Multi_Row_Selection#delete
    def delete
      Write_Query.new [:pointdelete, *(@body[1..-1])]
    end
  end
end
