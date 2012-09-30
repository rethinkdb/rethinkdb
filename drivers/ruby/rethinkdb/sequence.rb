module RethinkDB
  # A "Sequence" is either a JSON array or a stream.  The functions in
  # this module may be invoked as instance methods of both JSON_Expression and
  # Stream_Expression, but you will get a runtime error if you invoke
  # them on a JSON_Expression that turns out not to be an array.
  module Sequence
    # For each element of the sequence, execute 1 or more write queries (to
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
        Write_Query.new [:foreach, self, vname, queries]
      }
    end

    # Filter the sequence based on a predicate.  The provided block should take a
    # single variable, an element of the sequence, and return either <b>+true+</b> if
    # it should be in the resulting sequence or <b>+false+</b> otherwise.  For example:
    #   table.filter {|row| row[:id] < 5}
    # Alternatively, you may provide an object as an argument, in which case the
    # <b>+filter+</b> will match JSON objects which match the provided object's
    # attributes.  For example, if we have a table <b>+people+</b>, the
    # following are equivalent:
    #   people.filter{|row| row[:name].eq('Bob') & row[:age].eq(50)}
    #   people.filter({:name => 'Bob', :age => 50})
    # Note that the values of attributes may themselves be queries.  For
    # instance, here is a query that matches anyone whose age is double their height:
    #   people.filter({:age => r.mul(:height, 2)})
    def filter(obj=nil)
      if obj
        if obj.class == Hash         then self.filter { |row|
            JSON_Expression.new [:call, [:all], obj.map{|kv|
                                   row.getattr(kv[0]).eq(S.r(kv[1]))}]}
        elsif obj.kind_of? RQL_Query then self.filter {obj}
        else raise SyntaxError,"Filter: Not a hash or RQL query: #{obj.inspect}."
        end
      else
        S.with_var{|vname,v|
          self.class.new [:call, [:filter, vname, S.r(yield(v))], [self]]}
      end
    end

    # Map a function over the sequence, then concatenate the results together.  The
    # provided block should take a single variable, an element in the sequence, and
    # return a list of elements to include in the resulting sequence.  If you have a
    # table <b>+table+</b>, the following are all equivalent:
    #   table.concatmap {|row| [row[:id], row[:id]*2]}
    #   table.map{|row| [row[:id], row[:id]*2]}.reduce([]){|a,b| r.union(a,b)}
    def concatmap
      S.with_var { |vname,v|
        self.class.new [:call, [:concatmap, vname, S.r(yield(v))], [self]]}
    end

    # Gets all rows with keys between <b>+start_key+</b> and
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
      start_key = S.r(start_key || S.skip)
      end_key = S.r(end_key || S.skip)
      self.class.new [:call, [:range, keyname, start_key, end_key], [self]]
    end

    # Map a function over a sequence.  The provided block should take
    # a single variable, an element of the sequence, and return an
    # element of the resulting sequence.  For example:
    #   table.map {|row| row[:id]}
    def map
      S.with_var{|vname,v|
        self.class.new [:call, [:map, vname, S.r(yield(v))], [self]]}
    end

    # For each element of a sequence, picks out the specified
    # attributes from the object and returns only those.
    # If the input is not an array, fails when the query is run.
    #   expr([{ 'a' => 1, 'b' => 1, 'c' => 1},
    #         { 'a' => 2, 'b' => 2, 'c' => 2}]).pluck('a', 'b').run()
    #   [{ 'a' => 1, 'b' => 1 }, { 'a' => 2, 'b' => 2 }]
    def pluck(*args)
      self.map {|x| x.pickattrs(*args)}
    end

    # Order a sequence of objects by one or more attributes.  For
    # example, to sort first by name and then by social security
    # number for the table <b>+people+</b>, you could do:
    #   people.orderby(:name, :ssn)
    # In place of an attribute name, you may provide a tuple of an attribute
    # name and a boolean specifying whether to sort in ascending order (which is
    # the default).  For example:
    #   people.orderby([:name, false], :ssn)
    # will sort first by name in descending order, and then by ssn in ascending
    # order.
    def orderby(*orderings)
      orderings.map!{|x| x.class == Array ? x : [x, true]}
      self.class.new [:call, [:orderby, *orderings], [self]]
    end

    # Reduce a function over the sequence.  Note that unlike Ruby's reduce, you
    # cannot omit the base case.  The block you provide should take two
    # arguments, just like Ruby's reduce.  For example, if we have a table
    # <b>+table+</b>, the following will add up the <b>+count+</b> attribute of
    # all the rows:
    #   table.map{|row| row[:count]}.reduce(0) {|a,b| a+b}
    # <b>NOTE:</b> unlike Ruby's reduce, this reduce only works on
    # sequences with elements of the same type as the base case.  For
    # example, the following is incorrect:
    #   table.reduce(0){|a,b| a + b[:count]}
    # because the base case is a number but the sequence contains
    # objects.  RQL reduce has this limitation so that it can be
    # distributed across shards efficiently.
    def reduce(base)
      S.with_var { |aname,a|
        S.with_var { |bname,b|
          JSON_Expression.new [:call,
                               [:reduce, S.r(base), aname, bname, S.r(yield(a,b))],
                               [self]]}}
    end

    # This one is a little complicated.  The logic is as follows:
    # 1. Use <b>+grouping+</b> sort the elements into groups.  <b>+grouping+</b> should be a callable that takes one argument, the current element of the sequence, and returns a JSON expression representing its group.
    # 2. Map <b>+mapping+</b> over each of the groups.  Mapping should be a callable that behaves the same as the block passed to Sequence#map.
    # 3. Reduce the groups with <b>+base+</b> and <b>+reduction+</b>.  Base should be the base term of the reduction, and <b>+reduction+</b> should be a callable that behaves the same as the block passed to Sequence#reduce.
    #
    # For example, the following are equivalent:
    #   table.groupedmapreduce(lambda {|row| row[:id] % 4},
    #                          lambda {|row| row[:id]},
    #                          0,
    #                          lambda {|a,b| a+b})
    #   r[[0,1,2,3]].map {|n|
    #     table.filter{|row| row[:id].eq(n)}.map{|row| row[:id]}.reduce(0){|a,b| a+b}
    #   }
    # Groupedmapreduce is more efficient than the second form because
    # it only has to traverse <b>+table+</b> once.
    def groupedmapreduce(grouping, mapping, base, reduction)
      grouping_term = S.with_var{|vname,v| [vname, S.r(grouping.call(v))]}
      mapping_term = S.with_var{|vname,v| [vname, S.r(mapping.call(v))]}
      reduction_term = S.with_var {|aname, a| S.with_var {|bname, b|
          [S.r(base), aname, bname, reduction.call(a, b)]}}
      JSON_Expression.new [:call, [:groupedmapreduce,
                                   grouping_term,
                                   mapping_term,
                                   reduction_term],
                           [self]]
    end

    # Gets one or more elements from the sequence, much like [] in Ruby.
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
    # <b>NOTE:</b> If you are slicing an array, you can provide any negative index you
    # want, but if you're slicing a stream then for efficiency reasons the only
    # allowable negative index is '-1', and you must be using a closed range
    # ('..', not '...').
    def [](ind)
      case ind.class.hash
      when Fixnum.hash then
        JSON_Expression.new [:call, [:nth], [self, RQL.expr(ind)]]
      when Range.hash then
        b = RQL.expr(ind.begin)
        if ind.exclude_end? then e = ind.end
                            else e = (ind.end == -1 ? nil : RQL.expr(ind.end+1))
        end
        self.class.new [:call, [:slice], [self, RQL.expr(b), RQL.expr(e)]]
      else raise SyntaxError, "RQL_Query#[] can't handle #{ind.inspect}."
      end
    end

    # Return at most <b>+n+</b> elements from the sequence.  The
    # following are equivalent:
    #   r[[1,2,3]]
    #   r[[1,2,3,4]].limit(3)
    #   r[[1,2,3,4]][0...3]
    def limit(n); self[0...n]; end

    # Skip the first <b>+n+</b> elements of the sequence.  The following are equivalent:
    #   r[[2,3,4]]
    #   r[[1,2,3,4]].skip(1)
    #   r[[1,2,3,4]][1..-1]
    def skip(n); self[n..-1]; end

    # Removes duplicate values from the sequence (similar to the *nix
    # <b>+uniq+</b> function).  Does not work for sequences of
    # compound data types like objects or arrays, but in the case of
    # objects (e.g. rows of a table), you may provide an attribute and
    # it will first map the selector for that attribute over the
    # sequence.  If we have a table <b>+table+</b>, the following are
    # equivalent:
    #   table.map{|row| row[:id]}.distinct
    #   table.distinct(:id)
    # As are:
    #   r[[1,2,3]]
    #   r[[1,2,3,1]].distinct
    # And:
    #   r[[1,2]]
    #   r[[{:x => 1}, {:x => 2}, {:x => 1}]].distinct(:x)
    def distinct(attr=nil);
      if attr then self.map{|row| row[attr]}.distinct
              else self.class.new [:call, [:distinct], [self]];
      end
    end

    # Get the length of the sequence.  If we have a table
    # <b>+table+</b> with at least 5 elements, the following are
    # equivalent:
    #   table[0...5].length
    #   r[[1,2,3,4,5]].length
    def length(); JSON_Expression.new [:call, [:length], [self]]; end

    # Get element <b>+n+</b> of the sequence.  For example, the following are
    # equivalent:
    #   r[2]
    #   r[[0,1,2,3]].nth(2)
    # (Note the 0-indexing.)
    def nth(n)
      JSON_Expression.new [:call, [:nth], [self, S.r(n)]]
    end

    def innerjoin(other)
        self.concatmap { |row|
            other.concatmap { |row2|
                RethinkDB::RQL.if(yield(row, row2),
                    RethinkDB::RQL.expr([{:left => row, :right => row2}]),
                    RethinkDB::RQL.expr([])
                )
            }
        }
    end

    def outerjoin(other)
        self.concatmap { |row|
            RethinkDB::RQL.let({:matches => other.concatmap { |row2|
                    RethinkDB::RQL.if(yield(row, row2),
                        RethinkDB::RQL.expr([{:left => row, :right => row2}]),
                        RethinkDB::RQL.expr([])
                    )
                }
            }, RethinkDB::RQL.if(RethinkDB::RQL.letvar(:matches).length() > 0,
                    RethinkDB::RQL.letvar(:matches),
                    RethinkDB::RQL.expr([:left => row])
                )
            )
        }
    end

    def equijoin(leftattr, other, opt_rightattr=nil)
        self.concatmap { |row|
            RethinkDB::RQL.let({:right => other.get(row[leftattr])},
                RethinkDB::RQL.if(RethinkDB::RQL.letvar(:right).ne(nil),
                    RethinkDB::RQL.expr([{:left => row, :right => RethinkDB::RQL.letvar(:right)}]),
                    RethinkDB::RQL.expr([])
                )
            )
        }
    end

    def zip
        self.map {|row|
            RethinkDB::RQL.if(row.hasattr('right'),
                row['left'].mapmerge(row['right']),
                row['left']
            )
        }
    end

  end
end
