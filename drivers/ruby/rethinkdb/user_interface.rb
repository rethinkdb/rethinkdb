# Right now, this is a place to record high-level spec changes that need to
# happen.  TODO:
# * UPDATE needs to be changed to do an implicit mapmerge on its righthand side
#   (this will make its behavior line up with the Python documentation).
# * UPDATE needs to work if you return NIL
# * MUTATE needs to be renamed to REPLACE (Joe and Tim and I just talked about
#   this) and maintain its current behavior rather than changing to match
#   UPDATE.
# * The following are unimplemented: REDUCE, GROUPEDMAPREDUCE, POINTMUTATE
# * The following are buggy: UPDATE, POINTUPDATE, FOREACH
# * The following are going away: ARRAYNTH
# * I don't understand how GROUPEDMAPREDUCE works.
module RethinkDB
  # A network connection to the RethinkDB cluster.
  class Connection
    # Create a new connection to <b>+host+</b> on port <b>+port+</b>.  Example:
    #   c = Connection.new('localhost')
    def initialize(host, port=12346)
      @@last = self
      @socket = TCPSocket.open(host, port)
      @waiters = {}
      @data = {}
      @mutex = Mutex.new
      start_listener
    end

    # Run the RQL query <b>+query+</b>.  Returns either a list of JSON values or
    # a single JSON value depending on <b>+query+</b>.  (Note that JSON values
    # will be converted to Ruby datatypes by the time you receive them.)
    # Example (assuming a connection <b>+c+</b> and that you've mixed in
    # shortcut <b>+r+</b>):
    #   c.run(r.add(1,2))                         => 3
    #   c.run(r[[r.add(1,2)]])                    => [3]
    #   c.run(r[[r.add(1,2), r.add(10,20)]])      => [3, 30]
    def run query
      a = []
      token_iter(dispatch query){|row| a.push row} ? a : a[0]
    end

    # Run the RQL query <b>+query+</b> and iterate over the results.  The
    # <b>+block+</b> you provide should take a single argument, which will be
    # bound to a single JSON value each time your block is invoked.  Returns
    # <b>+true+</b> if your query returned a sequence of values or
    # <b>+false+</b> if your query returned a single value.  (Note that JSON
    # values will be converted to Ruby datatypes by the time you receive them.)
    # Example (assuming a connection <b>+c+</b> and that you've mixed in
    # shortcut <b>+r+</b>):
    #   a = []
    #   c.iter(r.add(1,2)) {|val| a.push val}                    => false
    #   a                                                        => [3]
    #   c.iter(r[[r.add(10,20)]]) {|val| a.push val} => true
    #   a                                                        => [3, 30]
    def iter(query, &block)
      token_iter(dispatch(query), &block)
    end

    # Close the connection.
    def close
      @listener.terminate!
      @socket.close
    end
  end

  # An RQL query that can be sent to the RethinkDB cluster.  Queries are either
  # constructed by methods in the RQL module, or by invoking the instance
  # methods of a query on other queries.
  #
  # Note: many things that look like instance methods of queries actually reside
  # in the RQL_Mixin module, and instance method variants are provided only for
  # convenience.  If you don't see your method here, check there.
  class RQL_Query
    # Create a table.  When run, either returns <b>+nil+</b>or raises.  For
    # example:
    #   r.db('Welcome-db','Welcome-rdb').create
    def create(datacenter=nil)
      S._ [:create, (datacenter || @@default_datacenter), @body]
    end

    # Drop a table.  When run, either returns <b>+nil+</b>or raises.  For
    # example:
    #   r.db('Welcome-db','Welcome-rdb').drop
    def drop(); S._ [:drop, *@body]; end

    # Convert from an RQL query representing a variable to the name of that
    # variable.  Used e.g. in constructing javascript functions.
    def to_s
      if @body.class == Array and @body[0] == :var
      then @body[1]
      else raise TypeError, 'Can only call to_s on RQL_Queries representing variables.'
      end
    end

    # TODO: only handle open ranges explicitly?
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
      when Fixnum.hash then S._ [:call, [:nth], [@body, RQL.expr(ind)]]
      when Range.hash then
        b = RQL.expr(ind.begin)
        #PP.pp ind.exclude_end?
        if ind.exclude_end? then e = ind.end
                            else e = (ind.end == -1 ? nil : RQL.expr(ind.end+1))
        end
        #e = ind.end == -1 ? nil : RQL.expr(ind.end)
        S._ [:call, [:slice], [@body, RQL.expr(b), RQL.expr(e)]]
      when Symbol.hash, String.hash then S._ [:call, [:getattr, ind], [@body]]
      else raise SyntaxError, "RQL_Query#[] can't handle #{ind}."
      end
    end

    # Delete all rows of the invoking query, which *must* be a table.  For
    # example, if we have a table
    # <b>+table+</b>:
    #   table.filter{|row| row[:id] < 5}.delete
    # will construct a query that deletes everything with <b>+id+</b> less than
    # 5 in <b>+table+</b>
    def delete
      @body[0]==:getbykey ? S._([:pointdelete, *(@body[1..-1])]) : S._([:delete, @body])
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
      if @body[0] == :getbykey then S.with_var {|vname,v|
          S._ [:pointupdate, *(@body[1..-1] + [[vname, RQL.expr(yield(v))]])]}
      else S.with_var{|vname,v| S._ [:update, @body, [vname, RQL.expr(yield(v))]]}
      end
    end

    # Replace all rows of the invoking query.  Unlike update, must return the
    # new row rather than an object containing attributes to be updated (may be
    # combined with <b>+mapmerge+</b> to achieve a similar effect to update).
    # May also return <b>+nil+</b> to delete the row.  For example, if we have a
    # table <b>+table+</b>, then:
    #   table.mutate{|row| if(row[:id] < 5, nil, row)}
    # will delete everything with id less than 5, but leave the other rows untouched.
    def mutate
      if @body[0] == :getbykey then S.with_var {|vname,v|
          S._ [:pointmutate, *(@body[1..-1] + [[vname, RQL.expr(yield(v))]])]}
      else S.with_var{|vname,v| S._ [:mutate, @body, [vname, RQL.expr(yield(v))]]}
      end
    end

    # Insert one or more rows into the invoking query, which *must* be a table.
    # Rows with duplicate primary key (usually :id) are ignored, with no
    # guarantees about which row 'wins'.  May also provide only one row to
    # insert.  For example, if we have a table <b>+table+</b>, the following are
    # equivalent:
    #   table.insert({:id => 1, :name => 'Bob'})
    #   table.insert([{:id => 1, :name => 'Bob'}])
    #   table.insert([{:id => 1, :name => 'Bob'}, {:id => 1, :name => 'Bob'}])
    def insert(rows)
      rows = [rows] if rows.class != Array
      S.with_var {|vname,v| S._ [:insert, @body, rows.map{|x| RQL.expr x}]}
    end

    # Insert a stream into the invoking query, which *must* be a table.  Often
    # used to insert one table into another.  For example:
    #   table.insertstream(table2.filter{|row| row[:id] < 5})
    # will insert every row in <b>+table2+</b> with id less than 5 into
    # <b>+table+</b>.
    def insertstream(stream)
      S.with_var {|vname,v| S._ [:insertstream, @body, stream]}
    end

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
        S._ [:foreach, @body, vname, queries.map{|x| RQL.expr x}]
      }
    end

    # Run the invoking query using the most recently opened connection.  See
    # Connection#run for more details.
    def run; connection_send :run; end
    # Run the invoking query and iterate over the results using the most
    # recently opened connection.  See Connection#iter for more details.
    def iter; connection_send :iter; end

    # Get the row of the invoking table with key <b>+key+</b>.  You may also
    # optionally specify the name of the attribute to use as your key
    # (<b>+keyname+</b>), but note that your table must be indexed by that
    # attribute.  Calling this function on anything except a table is an error:
    #   good = table.get(0)
    #   bad  = table.filter{|row| row[:name].eq('Bob')}.get(0)
    # TODO: get().delete() => pointdelete
    def get(key, keyname=:id)
      if @body[0] == :table then S._ [:getbykey, @body[1..-1], keyname, RQL.expr(key)]
                            else raise SyntaxError, "Get must be called on a table."
      end
    end
  end

  # This module may be included/extended to gain access to the RQL query
  # construction functions.  By far the most common way of gaining access to
  # those functions, however, is to include/extend RethinkDB::Shortcuts_Mixin to
  # gain access to the shortcut <b>+r+</b>.  That shortcut is used in all the
  # examples below.
  module RQL_Mixin
    # Construct a javascript expression, which may refer to variables in scope
    # (use <b>+to_s+</b> to get the name of a variable query, or simply splice
    # it in).  Defaults to a javascript expression, but if the optional second
    # argument is <b>+:func+</b>, then you may instead provide the body of a
    # javascript function.  Also has the shorter synonym <b>+js+</b>. If you
    # have a table <b>+table+</b>, the following
    # are equivalent:
    #   table.map{|row| row[:id]}
    #   table.map{|row| r.javascript("#{row}.id")}
    #   table.map{|row| r.js("#{row}.id")}
    #   table.map{r[:id]} #implicit variable
    #   table.map{r.js("this.id")} #implicit variable
    #   table.map{r.js("return this.id;", :func)} #implicit variable
    # As are:
    #   r.let([['a', 1],
    #          ['b', 2]],
    #         r.add(:$a, :$b, 1))
    #   r.let([['a', 1],
    #          ['b', 2]],
    #         r.js('a+b+1'))
    def javascript(str, type=:expr);
      if    type == :expr then S._ [:javascript, "return #{str}"]
      elsif type == :func then S._ [:javascript, str]
      else  raise TypeError, 'Type of javascript must be either :expr or :func.'
      end
    end

    # Return at most <b>+n+</b> elements from <b>+seq+</b>.  May also be called
    # as if it were a member function of RQL_Query for convenience.  The
    # following are equivalent:
    #   r[[1,2,3]].to_stream
    #   r.limit(r[[1,2,3,4]].to_stream, 3)
    #   r[[1,2,3,4]].to_stream.limit(3)
    #   r[[1,2,3,4]].to_stream[0...3]
    def limit(seq, n); seq[0...n]; end

    # Skip the first <b>+n+</b> elements from <b>+seq+</b>.  May also be called
    # as if it were a member function of RQL_Query for convenience.  The
    # following are equivalent:
    #   r[[2,3,4]].to_stream
    #   r.skip(r[[1,2,3,4]].to_stream, 1)
    #   r[[1,2,3,4]].to_stream.skip(1)
    #   r[[1,2,3,4]].to_stream[1..-1]
    def skip(seq, n); seq[n..-1]; end

    # Construct a new table reference, which may then be treated as a query for
    # chaining (see the functions in RQL_Query).  There are two identical ways
    # of getting access to a namespace: passing it as the second argument, or
    # calling it as an instance method if no second argument was provided.  For
    # instance, to access the namespace Welcome, either of these works:
    #   db('','Welcome')
    #   db('').Welcome
    def db(db_name, table_name=nil); Tbl.new db_name, table_name; end

    # A shortcut for RQL_Mixin#expr.  The following are equivalent:
    #   r.expr(5)
    #   r[5]
    def [](ind); expr ind; end

    # Convert from a Ruby datatype to an RQL query.  More commonly accessed
    # with its shortcut, <b><tt>[]</tt></b>.  Numbers, strings, booleans,
    # arrays, objects, and nil are all converted to their respective JSON types.
    # Symbols that begin with '$' are converted to variables, and symbols
    # without a '$' are converted to attributes of the implicit variable.  The
    # implicit variable is defined for most operations as the current row.  For
    # example, the following are equivalent:
    #   r.db('').Welcome.filter{|row| row[:id].eq(5)}
    #   r.db('').Welcome.filter{r.expr(:id).eq(5)}
    #   r.db('').Welcome.filter{r[:id].eq(5)}
    # Variables are used in e.g. <b>+let+</b> bindings.  For example:
    #   r.let([['a', 1],
    #          ['b', r[:$a]+1]],
    #         r[:$b]*2).run
    # will return 4.  (This notation will usually result in editors highlighting
    # your variables the same as global ruby variables, which makes them stand
    # out nicely in queries.)
    #
    # Note: this function is idempotent, so the following are equivalent:
    #   r[5]
    #   r[r[5]]
    # Note 2: the implicit variable can be dangerous.  Never pass it to a
    # Ruby function, as it might enter a different scope without your
    # knowledge.  In general, if you're doing something complicated, consider
    # naming your variables.
    def expr x
      case x.class().hash
      when RQL_Query.hash  then x
      when String.hash     then S._ [:string, x]
      when Fixnum.hash     then S._ [:number, x]
      when Float.hash      then S._ [:number, x]
      when TrueClass.hash  then S._ [:bool, x]
      when FalseClass.hash then S._ [:bool, x]
      when NilClass.hash   then S._ [:json_null]
      when Array.hash      then S._ [:array, *x.map{|y| expr(y)}]
      when Hash.hash       then S._ [:object, *x.map{|var,term| [var, expr(term)]}]
      when Symbol.hash     then S._ x.to_s[0]=='$'[0] ? var(x.to_s[1..-1]) : getattr(x)
      when Tbl.hash        then x.expr
      else raise TypeError, "RQL.expr can't handle '#{x.class()}'"
      end
    end

    # Explicitly construct an RQL variable from a string.  The following are
    # equivalent:
    #   r[:$varname]
    #   r.var('varname')
    def var(varname); S._ [:var, varname]; end

    # Provide a literal JSON string that will be parsed by the server.  For
    # example, the following are equivalent:
    #   r.expr([1,2,3])
    #   r.json('[1,2,3]')
    def json(str); S._ [:json, str]; end

    # Construct an error.  This is usually used in the branch of an <b>+if+</b>
    # expression.  For example:
    #   r.if(r[1] > 2, false, r.error('unreachable'))
    # will only run the error query if 1 is greater than 2.  If an error query
    # does get run, it will be received as a RuntimeError in Ruby, so be
    # prepared to handle it.
    def error(err); S._ [:error, err]; end

    # Construct a query that runs a subquery <b>+test+</b> which returns a
    # boolean, then branches into either <b>+t_branch+</b> if <b>+test+</b>
    # returned true or <b>+f_branch+</b> if <b>+test+</b> returned false.  For
    # example, if we have a table <b>+table+</b>:
    #   table.update{|row| if(row[:score] < 10, {:score => 10}, {})}
    # will change every row with score below 10 in <b>+table+</b> to have score 10.
    def if(test, t_branch, f_branch)
      S._ [:if, expr(test), expr(t_branch), expr(f_branch)]
    end

    # Construct a query that binds some values to variable (as specified by
    # <b>+varbinds+</b>) and then executes <b>+body+</b> with those variables in
    # scope.  For example:
    #   r.let([['a', 2],
    #          ['b', r[:$a]+1]],
    #         r[:$b]*2)
    # will bind <b>+a+</b> to 2, <b>+b+</b> to 3, and then return 6.  (It is
    # thus analagous to <b><tt>let*</tt></b> in the Lisp family of languages.)
    # It may also be used with hashes instead of lists, but in that case you
    # give up some power: in particular, you cannot have variables that depend
    # on previous variables, because the order is not guaranteed.  For example:
    #   r.let({:a => 2, :b => 3}, r[:$b]*2) # legal
    #   r.let({:a => 2, :b => r[:$a]+1}, r[:$b]*2) #legality not guaranteed
    def let(varbinds, body);
      varbinds.map! { |pair|
        raise SyntaxError,"Malformed LET expression #{body}" if pair.length != 2
        [pair[0].to_s, expr(pair[1])]}
      S._ [:let, varbinds, expr(body)]; end

    # Negate a predicate.  May also be called as if it were a instance method of
    # RQL_Query for convenience.  The following are equivalent:
    #   r.not(true)
    #   r[true].not
    def not(pred); S._ [:call, [:not], [expr pred]]; end

    # Explicitly construct a reference to an attribute of the implicit
    # variable.  This is useful if for some reason you named an attribute that's
    # hard to express as a symbol, or an attribute starting with a '$'.  The
    # following are equivalent:
    #   r[:attrname]
    #   r.attr('attrname')
    # Get an attribute of the implicit variable (usually done with
    # RQL_Mixin#expr).  Getting an attribute explicitly is useful if it has an
    # odd name that can't be expressed as a symbol (or a name that starts with a
    # $).  Synonyms are <b>+get+</b> and <b>+attr+</b>.  The following are all
    # equivalent:
    #   r[:id]
    #   r.expr(:id)
    #   r.getattr('id')
    #   r.get('id')
    #   r.attr('id')
    def getattr(obj, attrname=nil)
      if not attrname then S._ [:call, [:implicit_getattr, obj], []]
                      else S._ [:call, [:getattr, attrname], [obj]]
      end
    end

    # Test whether the implicit variable has a particular attribute.  Synonyms
    # are <b>+has+</b> and <b>+attr?+</b>.  The following are all equivalent:
    #   r.hasattr('name')
    #   r.has('name')
    #   r.attr?('name')
    def hasattr(obj, attrname=nil)
      if not attrname then S._ [:call, [:implicit_hasattr, obj], []]
                      else S._ [:call, [:hasattr, attrname], [obj]]
      end
    end

    # Construct an object that is a subset of the object stored in the implicit
    # variable by extracting certain attributes.  Synonyms are <b>+pick+</b> and
    # <b>+attrs+</b>.  The following are all equivalent:
    #   r[{:id => r[:id], :name => r[:name]}]
    #   r.pickattrs(:id, :name)
    #   r.pick(:id, :name)
    #   r.attrs(:id, :name)
    def pickattrs(*attrnames)
      if attrnames[0].class != RQL_Query
      then S._ [:call, [:implicit_pickattrs, *attrnames], []]
      else S._ [:call, [:pickattrs, *(attrnames[1..-1])], [attrnames[0]]]
      end
    end

    # Add the results of two or more queries together.  (Those queries should
    # return numbers.)  May also be called as if it were a instance method of
    # RQL_Query for convenience, and overloads <b><tt>+</tt></b> if the lefthand
    # side is a query.  The following are all equivalent:
    #   r.add(1,2)
    #   r[1].add(2)
    #   (r[1] + 2) # Note that (1 + r[2]) is *incorrect* because Ruby only
    #              # overloads based on the lefthand side.
    # The following is also legal:
    #   r.add(1,2,3)
    # Add may also be used to concatenate arrays.  The following are equivalent:
    #   r[[1,2,3]]
    #   r.add([1, 2], [3])
    def add(a, b, *rest)
      S._ [:call, [:add], [expr(a), expr(b), *(rest.map{|x| expr x})]];
    end

    # Subtract one query from another.  (Those queries should return numbers.)
    # May also be called as if it were a instance method of RQL_Query for
    # convenience, and overloads <b><tt>-</tt></b> if the lefthand side is a
    # query.  Also has the shorter synonym <b>+sub+</b>. The following are all
    # equivalent:
    #   r.subtract(1,2)
    #   r[1].subtract(2)
    #   r.sub(1,2)
    #   r[1].sub(2)
    #   (r[1] - 2) # Note that (1 - r[2]) is *incorrect* because Ruby only
    #              # overloads based on the lefthand side.
    def subtract(a, b); S._ [:call, [:subtract], [expr(a), expr(b)]]; end

    # Multiply the results of two or more queries together.  (Those queries should
    # return numbers.)  May also be called as if it were a instance method of
    # RQL_Query for convenience, and overloads <b><tt>+</tt></b> if the lefthand
    # side is a query.  Also has the shorter synonym <b>+mul+</b>.  The
    # following are all equivalent:
    #   r.multiply(1,2)
    #   r[1].multiply(2)
    #   r.mul(1,2)
    #   r[1].mul(2)
    #   (r[1] * 2) # Note that (1 * r[2]) is *incorrect* because Ruby only
    #              # overloads based on the lefthand side.
    # The following is also legal:
    #   r.multiply(1,2,3)
    def multiply(a, b, *rest)
      S._ [:call, [:multiply], [expr(a), expr(b), *(rest.map{|x| expr x})]];
    end

    # Divide one query by another.  (Those queries should return numbers.)
    # May also be called as if it were a instance method of RQL_Query for
    # convenience, and overloads <b><tt>/</tt></b> if the lefthand side is a
    # query.  Also has the shorter synonym <b>+div+</b>. The following are all
    # equivalent:
    #   r.divide(1,2)
    #   r[1].divide(2)
    #   r.div(1,2)
    #   r[1].div(2)
    #   (r[1] / 2) # Note that (1 / r[2]) is *incorrect* because Ruby only
    #              # overloads based on the lefthand side.
    def divide(a, b); S._ [:call, [:divide], [expr(a), expr(b)]]; end

    # Take one query modulo another.  (Those queries should return numbers.)
    # May also be called as if it were a instance method of RQL_Query for
    # convenience, and overloads <b><tt>%</tt></b> if the lefthand side is a
    # query.  Also has the shorter synonym <b>+mod+</b>. The following are all
    # equivalent:
    #   r.modulo(1,2)
    #   r[1].modulo(2)
    #   r.mod(1,2)
    #   r[1].mod(2)
    #   (r[1] % 2) # Note that (1 % r[2]) is *incorrect* because Ruby only
    #              # overloads based on the lefthand side.
    def modulo(a, b); S._ [:call, [:modulo], [expr(a), expr(b)]]; end

    # Take one or more predicate queries and construct a query that returns true
    # if any of them evaluate to true.  Sort of like <b>+or+</b> in ruby, but
    # takes arbitrarily many arguments and is *not* guaranteed to
    # short-circuit.  May also be called as if it were a instance method of
    # RQL_Query for convenience, and overloads <b><tt>|</tt></b> if the lefthand
    # side is a query.  Also has the synonym <b>+or+</b>.  The following are
    # all equivalent:
    #   r[true]
    #   r.any(false, true)
    #   r.or(false, true)
    #   r[false].any(true)
    #   r[false].or(true)
    #   (r[false] | true) # Note that (false | r[true]) is *incorrect* because
    #                     # Ruby only overloads based on the lefthand side
    def any(pred, *rest)
      S._ [:call, [:any], [expr(pred), *(rest.map{|x| expr x})]];
    end

    # Take one or more predicate queries and construct a query that returns true
    # if all of them evaluate to true.  Sort of like <b>+and+</b> in ruby, but
    # takes arbitrarily many arguments and is *not* guaranteed to
    # short-circuit.  May also be called as if it were a instance method of
    # RQL_Query for convenience, and overloads <b><tt>&</tt></b> if the lefthand
    # side is a query.  Also has the synonym <b>+and+</b>.  The following are
    # all equivalent:
    #   r[false]
    #   r.all(false, true)
    #   r.and(false, true)
    #   r[false].all(true)
    #   r[false].and(true)
    #   (r[false] & true) # Note that (false & r[true]) is *incorrect* because
    #                     # Ruby only overloads based on the lefthand side
    def all(pred, *rest)
      S._ [:call, [:all], [expr(pred), *(rest.map{|x| expr x})]];
    end

    # Filter a query returning a stream, based on a predicate.  May also be
    # called as if it were a instance method of RQL_Query for convenience.  The
    # provided block should take a single variable, a row in the stream, and
    # return either <b>+true+</b> if it should be in the resulting stream of
    # <b>+false+</b> otherwise.  If you have a table <b>+table+</b>, the
    # following are all equivalent:
    #   r.filter(table) {|row| row[:id] < 5}
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
    # --
    # (not currently true)
    # WARNING: since the implicit variable always refers to the nearest
    # enclosing block, if you were to use the implicit variable in a filter
    # specified by an dobject, you would instead access the nearest enclosing
    # block.  The following, for example, are equivalent:
    #   table.concatmap{people.filter({:age => r[:height]*2})}
    #   table.concatmap{|row| people.filter({:age => row[:height]*2})}
    def filter(stream, obj=nil)
      if obj
        raise SyntaxError,"Filter: Not a hash: #{obj.inspect}." if obj.class != Hash
        filter(stream) {
          S._ [:call, [:all], obj.map{|kv| getattr(kv[0]).eq(expr(kv[1]))}]
        }
      else
        S.with_var{|vname,v| S._ [:call,[:filter, vname, expr(yield(v))], [expr stream]]}
      end
    end

    # Map a function over a query returning a stream.  May also be called as if
    # it were a instance method of RQL_Query for convenience.  The provided
    # block should take a single variable, a row in the stream, and return a row
    # in the resulting stream.  If you have a table <b>+table+</b>, the
    # following are all equivalent:
    #   r.map(table) {|row| row[:id]}
    #   table.map {|row| row[:id]}
    #   table.map {r[:id]} # uses implicit variable
    def map(stream)
      S.with_var{|vname,v| S._ [:call, [:map, vname, expr(yield(v))], [expr(stream)]]}
    end

    # Map a function over a query returning a stream, then concatenate the
    # results together.  May also be called as if it were a instance method of
    # RQL_Query for convenience.  The provided block should take a single
    # variable, a row in the stream, and return a list of rows to include in the
    # resulting stream.  If you have a table <b>+table+</b>, the following are
    # all equivalent:
    #   r.concatmap(table) {|row| [row[:id], row[:id]*2]}
    #   table.concatmap {|row| [row[:id], row[:id]*2]}
    #   table.concatmap {[r[:id], r[:id]*2]} # uses implicit variable
    #   table.map{|row| [row[:id], row[:id]*2]}.reduce([]){|a,b| r.union(a,b)}
    def concatmap(stream)
      S.with_var { |vname,v|
        S._ [:call, [:concatmap, vname, expr(yield(v))], [expr stream]]
      }
    end

    # TODO: start_inclusive, end_inclusive in python -- what do these do?
    #
    # Construct a query which yields all rows of <b>+stream+</b> with keys
    # between <b>+start_key+</b> and <b>+end_key+</b> (inclusive).  You may also
    # optionally specify the name of the attribute to use as your key
    # (<b>+keyname+</b>), but note that your table must be indexed by that
    # attribute.  Either <b>+start_key+</b> or <b>+end_key+</b> may be nil, in
    # which case that side of the range is unbounded.  This function may also be
    # called as if it were a instance method of RQL_Query, for convenience.  For
    # example, if we have a table <b>+table+</b>, these are equivalent:
    #   r.between(table, 3, 7)
    #   table.between(3,7)
    #   table.filter{|row| (row[:id] >= 3) & (row[:id] <= 7)}
    # as are these:
    #   table.between(nil,7,:index)
    #   table.filter{|row| row[:index] <= 7}
    def between(stream, start_key, end_key, keyname=:id)
      opts = {:attrname => keyname}
      opts[:lowerbound] = (expr start_key).sexp if start_key != nil
      opts[:upperbound] = (expr end_key).sexp   if end_key   != nil
      S._ [:call, [:range, opts], [expr stream]]
    end

    # Removes duplicate items from <b>+seq+</b>, which may be either a JSON
    # array or a stream (similar to the *nix <b>+uniq+</b> function).  Does not
    # work for sequences of compound data types like objects or arrays, but in
    # the case of objects (e.g. rows of a table), you may provide an attribute
    # and it will first maps the selector for that attribute over the object.
    # May also be called as if it were a instance method of RQL_Query, for
    # convenience.  If we have a table <b>+table+</b>, the following are
    # equivalent:
    #   r.distinct(table, :id)
    #   table.distinct(:id)
    # As are:
    #   r[[1,2,3]]
    #   r.distinct([1,2,3,1])
    #   r[[1,2,3,1]].distinct
    # And:
    #   r[[1,2]]
    #   r[[{:x => 1}, {:x => 2}, {:x => 1}]].to_stream.distinct(:x)
    def distinct(seq, attr=nil);
      if attr then distinct(map(seq){|row| row[attr]})
              else S._ [:call, [:distinct], [expr seq]];
      end
    end

    # Get the length of <b>+seq+</b>, which may be either a JSON array or a
    # stream.  If we have a table <b>+table+</b> with at least 5 elements, the
    # following are equivalent:
    #   r.length(table[0...5])
    #   table[0...5].length
    #   r.length([1,2,3,4,5])
    #   r[[1,2,3,4,5]].length
    def length(seq); S._ [:call, [:length], [expr seq]]; end

    # Take the union of 0 or more sequences <b>+seqs+</b>.  Note that unlike
    # mathematical union, duplicate values are preserved.  May be called on only
    # on streams; use <b>+add+</b> for arrays.  May also be called as if it were
    # a instance method of RQL_Query, for convenience.  For example, if we have
    # a table <b>+table+</b>, the following are equivalent:
    #   r.union(table.map{r[:id]}, table.map{r[:num]})
    #   table.map{r[:id]}.union(table.map{r[:num]})
    def union(*seqs); S._ [:call, [:union], seqs.map{|x| expr x}]; end

    # Convert from an array to a stream.  Also has the synonym
    # <b>+to_stream+</b>.  May also be called as if it were a instance method of
    # RQL_Query, for convenience.  While most sequence functions are polymorphic
    # and handle both arrays and streams, when arrays or streams need to be
    # combined (e.g. via <b>+union+</b>) you need to explicitly convert between
    # the types.  This is mostly for safety, but also because which type you're
    # working with effects error handling.  The following are equivalent:
    #   r.arraytostream r.expr([1,2,3])
    #   r[[1,2,3]].arraytostream
    #   r.to_stream([1,2,3])
    #   r[[1,2,3]].to_stream
    def arraytostream(array); S._ [:call, [:arraytostream], [expr(array)]]; end

    # Convert from a stream to an array.  Also has the synonym <b>+to_array+</b>.
    # May also be called as if it were a instance method of RQL_Query, for
    # convenience.  While most sequence functions are polymorphic and handle
    # both arrays and streams, when arrays or streams need to be combined
    # (e.g. via <b>+union+</b>) you need to explicitly convert between the
    # types.  This is mostly for safety, but also because which type you're
    # working with effects error handling.  The following are equivalent:
    #   r.streamtoarray table
    #   table.streamtoarray
    #   r.to_array table
    #   table.to_array
    def streamtoarray(array); S._ [:call, [:streamtoarray], [array]]; end

    # Reduce a function over a stream.  Note that unlike Ruby's reduce, you
    # cannot omit the base case.  The block you privide should take two
    # arguments, just like Ruby's reduce.  May also be called as if it were an
    # instance method of RQL_Query, for convenience.  For example, if we have a
    # table <b>+table+</b>, the following are equivalent:
    #   r.reduce(r.map(table){|row| row[:count]}, 0, {|a,b| a+b})
    #   r.map(table){|row| row[:count]}.reduce(0) {|a,b| a+b}
    # TODO: actual tests (reduce unimplemented right now)
    def reduce(stream, base)
      S.with_var { |aname,a|
        S.with_var { |bname,b|
          S._ [:call,
               [:reduce, expr(base), aname, bname, expr(yield(a,b))],
               [expr(stream)]]
        }
      }
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
    def orderby(stream, *orderings)
      orderings.map!{|x| x.class == Array ? x : [x, true]}
      S._ [:call, [:orderby, *orderings], [expr(stream)]]
    end

    # Get element <b>+n+</b> of <b>+seq+</b>, which may be either a stream or a
    # JSON array.  May also be called as if it were an instance method of
    # RQL_Query, for convenience.  For example, if we have a table
    # <b>+table+</b> the following are equivalent:
    #   r.nth(table, 5)
    #   table.nth(5)
    # As are:
    #   r[2]
    #   r.nth([0,1,2,3], 2)
    #   r[[0,1,2,3]].nth(2)
    # (Note the 0-indexing.)
    def nth(seq, n); S._ [:call, [:nth], [expr(seq), expr(n)]]; end

    # Merge two objects together with a preference for the object on the right.
    # The resulting object has all the attributes in both objects, and if the
    # two objects share an attribute, the value from the object on the left
    # "wins" and is included in the final object.  May also be called as if it
    # were an instance method of RQL_Query, for convenience.  The following are
    # equivalent:
    #   r[{:a => 10, :b => 2, :c => 30}]
    #   r.mapmerge({:a => 1, :b => 2}, {:a => 10, :c => 30})
    #   r[{:a => 1, :b => 2}].mapmerge({:a => 10, :c => 30})
    def mapmerge(obj1, obj2); S._ [:call, [:mapmerge], [expr(obj1), expr(obj2)]]; end

    # Check whether the results of two queries are equal.  May also be called as
    # if it were a member function of RQL_Query for convenience.  Has the
    # synonym <b>+equals+</b>.  The following are all equivalent:
    #   r[true]
    #   r.eq 1,1
    #   r[1].eq(1)
    #   r.equals 1,1
    #   r[1].equals(1)
    # May also be used with more than two arguments.  The following are
    # equivalent:
    #   r[false]
    #   r.eq(1, 1, 2)
    def eq(*args); S._ [:call, [:compare, :eq], args.map{|x| expr x}]; end

    # Check whether the results of two queries are *not* equal.  May also be
    # called as if it were a member function of RQL_Query for convenience.  The
    # following are all equivalent:
    #   r[true]
    #   r.ne 1,2
    #   r[1].ne(2)
    #   r.not r.eq(1,2)
    #   r[1].eq(2).not
    # May also be used with more than two arguments.  The following are
    # equivalent:
    #   r[true]
    #   r.ne(1, 1, 2)
    def ne(*args); S._ [:call, [:compare, :ne], args.map{|x| expr x}]; end

    # Check whether the result of one query is less than another.  May also be
    # called as if it were a member function of RQL_Query for convenience.  May
    # also be called as the infix operator <b><tt> < </tt></b> if the lefthand
    # side is an RQL query.  The following are all equivalent:
    #   r[true]
    #   r.lt 1,2
    #   r[1].lt(2)
    #   r[1] < 2
    # Note that the following is illegal, because Ruby only overloads infix
    # operators based on the lefthand side:
    #   1 < r[2]
    # May also be used with more than two arguments.  The following are
    # equivalent:
    #   r[true]
    #   r.lt(1, 2, 3)
    def lt(*args); S._ [:call, [:compare, :lt], args.map{|x| expr x}]; end

    # Check whether the result of one query is less than or equal to another.
    # May also be called as if it were a member function of RQL_Query for
    # convenience.  May also be called as the infix operator <b><tt> <= </tt></b>
    # if the lefthand side is an RQL query.  The following are all equivalent:
    #   r[true]
    #   r.le 1,1
    #   r[1].le(1)
    #   r[1] <= 1
    # Note that the following is illegal, because Ruby only overloads infix
    # operators based on the lefthand side:
    #   1 <= r[1]
    # May also be used with more than two arguments.  The following are
    # equivalent:
    #   r[true]
    #   r.le(1, 2, 2)
    def le(*args); S._ [:call, [:compare, :le], args.map{|x| expr x}]; end

    # Check whether the result of one query is greater than another.
    # May also be called as if it were a member function of RQL_Query for
    # convenience.  May also be called as the infix operator <b><tt> > </tt></b>
    # if the lefthand side is an RQL query.  The following are all equivalent:
    #   r[true]
    #   r.gt 2,1
    #   r[2].gt(1)
    #   r[2] > 1
    # Note that the following is illegal, because Ruby only overloads infix
    # operators based on the lefthand side:
    #   2 > r[1]
    # May also be used with more than two arguments.  The following are
    # equivalent:
    #   r[true]
    #   r.gt(3, 2, 1)
    def gt(*args); S._ [:call, [:compare, :gt], args.map{|x| expr x}]; end

    # Check whether the result of one query is greater than or equal to another.
    # May also be called as if it were a member function of RQL_Query for
    # convenience.  May also be called as the infix operator <b><tt> >= </tt></b>
    # if the lefthand side is an RQL query.  The following are all equivalent:
    #   r[true]
    #   r.ge 1,1
    #   r[1].ge(1)
    #   r[1] >= 1
    # Note that the following is illegal, because Ruby only overloads infix
    # operators based on the lefthand side:
    #   1 >= r[1]
    # May also be used with more than two arguments.  The following are
    # equivalent:
    #   r[true]
    #   r.ge(2, 2, 1)
    def ge(*args); S._ [:call, [:compare, :ge], args.map{|x| expr x}]; end

    # Append a single element to an array.  May also be called s if it were a
    # member function of RQL_Query for convenience.  Has the shorter synonym
    # <b>+append+</b> The following are equivalent:
    #   r[[1,2,3,4]]
    #   r.arrayappend([1,2,3], 4)
    #   r[[1,2,3]].arrayappend(4)
    #   r.append([1,2,3], 4)
    #   r[[1,2,3]].append(4)
    def arrayappend(arr, el); S._ [:call, [:arrayappend], [expr(arr), expr(el)]]; end

    # List all the tables currently in the cluster.
    def list(); S._ [:list]; end
  end
end
