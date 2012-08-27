module RethinkDB
  # A query that doesn't modify any data.
  class Expression < RQL_Query
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
