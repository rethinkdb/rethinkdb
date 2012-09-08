module RethinkDB
  # A query that doesn't modify any data.
  class Expression < RQL_Query
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
        self.class.new [:call, [:concatmap, vname, S.r(yield(v))], [@body]]}
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
        self.class.new [:call, [:map, vname, S.r(yield(v))], [@body]]}
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
      self.class.new [:call, [:orderby, *orderings], [@body]]
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
        S.with_var { |bname,b|
          JSON_Expression.new [:call,
                               [:reduce, S.r(base), aname, bname, S.r(yield(a,b))],
                               [@body]]}}
    end

    # TODO: doc
    def groupedmapreduce(grouping, mapping, base, reduction)
      grouping_term = S.with_var{|vname,v| [vname, S.r(grouping.call(v))]}
      mapping_term = S.with_var{|vname,v| [vname, S.r(mapping.call(v))]}
      reduction_term = S.with_var {|aname, a| S.with_var {|bname, b|
          [S.r(base), aname, bname, reduction.call(a, b)]}}
      JSON_Expression.new [:call, [:groupedmapreduce,
                                   grouping_term,
                                   mapping_term,
                                   reduction_term],
                           [@body]]
    end

    # Much like the [] operator in normal Ruby, [] can be used on either objects
    # (e.g. rows, to get their attributes) or on sequences (e.g. arrays).  In
    # the latter case, you may provide either a single number in order to get an
    # element of the sequence, or a range in order to get a subsequence.
    # The following are all equivalent:
    #   r[[1,2,3]]
    #   r[[0,1,2,3]][1...4]
    #   r[[0,1,2,3]][1..3]
    #   r[[0,1,2,3]][1..-1]
    # As are:
    #   r[1]
    #   r[[0,1,2]][1]
    # And:
    #   r[2]
    #   r[{:a => 2}][:a]
    # NOTE: If you are slicing an array, you can provide any negative index you
    # want, but if you're slicing a stream then for efficiency reasons the only
    # allowable negative index is '-1', and you must be using a closed range
    # ('..', not '...').
    def [](ind)
      case ind.class.hash
      when Fixnum.hash then
        JSON_Expression.new [:call, [:nth], [@body, RQL.expr(ind)]]
      when Range.hash then
        b = RQL.expr(ind.begin)
        #PP.pp ind.exclude_end?
        if ind.exclude_end? then e = ind.end
                            else e = (ind.end == -1 ? nil : RQL.expr(ind.end+1))
        end
        self.class.new [:call, [:slice], [@body, RQL.expr(b), RQL.expr(e)]]
      when Symbol.hash, String.hash then
        JSON_Expression.new [:call, [:getattr, ind], [@body]]
      else raise SyntaxError, "RQL_Query#[] can't handle #{ind}."
      end
    end

    # Return at most <b>+n+</b> elements from the invoking query, which must be
    # a sequence.  The following are equivalent:
    #   r[[1,2,3]].to_stream
    #   r[[1,2,3,4]].to_stream.limit(3)
    #   r[[1,2,3,4]].to_stream[0...3]
    def limit(n); self[0...n]; end

    # Skip the first <b>+n+</b> elements of the invoking query, which must be a
    # sequence.  The following are equivalent:
    #   r[[2,3,4]].to_stream
    #   r[[1,2,3,4]].to_stream.skip(1)
    #   r[[1,2,3,4]].to_stream[1..-1]
    def skip(n); self[n..-1]; end

    # Removes duplicate items from invoking sequence, which may be either a JSON
    # array or a stream (similar to the *nix <b>+uniq+</b> function).  Does not
    # work for sequences of compound data types like objects or arrays, but in
    # the case of objects (e.g. rows of a table), you may provide an attribute
    # and it will first maps the selector for that attribute over the object.
    # If we have a table <b>+table+</b>, the following are equivalent:
    #   table.distinct
    #   table.distinct(:id)
    # As are:
    #   r[[1,2,3]]
    #   r[[1,2,3,1]].distinct
    # And:
    #   r[[1,2]]
    #   r[[{:x => 1}, {:x => 2}, {:x => 1}]].to_stream.distinct(:x)
    def distinct(attr=nil);
      if attr then self.map{|row| row[attr]}.distinct
              else self.class.new [:call, [:distinct], [@body]];
      end
    end

    # Get the length of <b>+seq+</b>, which may be either a JSON array or a
    # stream.  If we have a table <b>+table+</b> with at least 5 elements, the
    # following are equivalent:
    #   table[0...5].length
    #   r[[1,2,3,4,5]].length
    def length(); JSON_Expression.new [:call, [:length], [@body]]; end

    # Get element <b>+n+</b> of invoking query.  For example, the following are
    # equivalent:
    #   r[2]
    #   r[[0,1,2,3]].nth(2)
    # (Note the 0-indexing.)
    def nth(n)
      JSON_Expression.new [:call, [:nth], [@body, S.r(n)]]
    end
  end
end

load 'streams.rb'
load 'jsons.rb'
