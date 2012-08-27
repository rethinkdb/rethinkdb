module RethinkDB
  # A query returning a JSON object
  class JSON_Expression < Expression
    # Append a single element to an array.  Has the shorter synonym
    # <b>+append+</b> The following are equivalent:
    #   r[[1,2,3,4]]
    #   r[[1,2,3]].arrayappend(4)
    #   r[[1,2,3]].append(4)
    def arrayappend(el)
      JSON_Expression.new [:call, [:arrayappend], [@body, S.r(el)]]
    end

    # Get an attribute of the invoking query.
    def getattr(attrname)
      JSON_Expression.new [:call, [:getattr, attrname], [@body]]
    end

    # Check whether the invoking query has a particular attribute.
    def hasattr(attrname)
      JSON_Expression.new [:call, [:hasattr, attrname], [@body]]
    end

    # Construct an object that has a subet of the attributes of the invoking query.
    def pickattrs(*attrnames)
      JSON_Expression.new [:call, [:pickattrs, *attrnames], [@body]]
    end

    # Convert from an array to a stream.  Also has the synonym
    # <b>+to_stream+</b>.  While most sequence functions are polymorphic
    # and handle both arrays and streams, when arrays or streams need to be
    # combined (e.g. via <b>+union+</b>) you need to explicitly convert between
    # the types.  This is mostly for safety, but also because which type you're
    # working with effects error handling.  The following are equivalent:
    #   r[[1,2,3]].arraytostream
    #   r[[1,2,3]].to_stream
    def arraytostream(); Stream_Expression.new [:call, [:arraytostream], [@body]]; end

    # Prefix -
    def -@; JSON_Expression.new [:call, [:subtract], [@body]]; end
    # Prefix + (inverse of prefix -)
    def +@; JSON_Expression.new [:call, [:add], [@body]]; end
  end
  # A query representing a variable.
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

  # A query that refers to a single row of a table
  class Single_Row_Selection < JSON_Expression
    # Analagous to Stream_Query#update
    def update
      S.with_var {|vname,v|
        Write_Query.new [:pointupdate, *(@body[1..-1] + [[vname, S.r(yield(v))]])]}
    end

    # Analagous to Stream_Query#mutate
    def mutate
      S.with_var {|vname,v|
        Write_Query.new [:pointmutate, *(@body[1..-1] + [[vname, S.r(yield(v))]])]}
    end

    # Analagous to Stream_Query#delete
    def delete
      Write_Query.new [:pointdelete, *(@body[1..-1])]
    end
  end
end
