# TODO: toplevel documentation
module RethinkDB
  # A network connection to the RethinkDB cluster.
  class Connection
    # Create a new connection to <b>+host+</b> on port <b>+port+</b>.
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
  # When the instance methods of queries are invoked on native Ruby datatypes
  # rather than other queries, the RethinkDB library will attempt to convert the
  # Ruby datatype to a query.  Numbers, strings, booleans, and nil will all be
  # converted to query types, but arrays and objects need to be converted
  # explicitly with either the <b>+expr+</b> or <b><tt>[]</tt></b> methods in
  # the RQL module.
  class RQL_Query
    # Construct a query that deletes all rows of the invoking query.  For
    # example, if we have a table <b>+table+</b>:
    #   table.filter{|row| row[:id] < 5}.delete
    # will construct a query that deletes everything with <b>+id+</b> less than
    # 5 in <b>+table+</b>
    def delete; S._ [:delete, @body]; end

    # Construct a query which updates all rows of the invoking query.  For
    # example, if we have a table <b>+table+</b>:
    #   table.filter{|row| row[:id] < 5}.update{|row| {:score => row[:score]*2}}
    # will construct a query that doubles the score of everything with
    # <b>+id+</b> less tahn 4.  If the object returned in <b>+update+</b>
    # has attributes which are not present in the original row, those attributes
    # will still be added to the new row.
    def update; with_var {|vname,v| S._ [:update, @body, [vname, yield(v)]]}; end

    #TODO: start_inclusive, end_inclusive in python -- what do these do?
    def between(start_key, end_key, keyname=:id)
      opts = {:attrname => keyname}
      opts[:lowerbound] = start_key if start_key != nil
      opts[:upperbound] = end_key if end_key != nil
      S._ [:call, [:range, opts], [@body]]
    end
  end
end
