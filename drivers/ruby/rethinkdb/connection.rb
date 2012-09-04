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

    #TODO: doc
    def run_async query
      dispatch query
    end

    #TODO: doc
    def block token
      a = []
      token_iter(token){|row| a.push row} ? a : a[0]
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
end
