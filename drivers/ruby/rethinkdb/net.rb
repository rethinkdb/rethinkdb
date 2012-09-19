require 'socket'
require 'thread'
require 'json'

module RethinkDB
  # The result of a calling Connection#run on a query that returns a stream.
  # This class is <b>+Enumerable+</b>.  You can find documentation on Enumerable
  # classes at http://ruby-doc.org/core-1.9.3/Enumerable.html .
  #
  # <b>NOTE:</b> unlike most enumerable objects, you can only iterate over Query
  # results once.  The results are fetched lazily from the server in chunks
  # of 1000.  If you need to access to values multiple times, use the <b>+to_a+</b>
  # instance method to get an Array, and then work with that.
  class Query_Results
    def initialize(connection, token) # :nodoc:
      @run = false
      @connection = connection
      @token = token
    end

    def each (&block) # :nodoc:
      raise RuntimeError, "Can only iterate over Query_Results once!" if @run
      @connection.token_iter(@token, &block)
      @run = true
      return self
    end
  end

  # TODO: Make sure tokens don't overflow.
  #
  # A connection to the RethinkDB
  # cluster.  You need to create at least one connection before you can run
  # queries.  After creating a connection <b>+conn+</b> and constructing a
  # query <b>+q+</b>, the following are equivalent:
  #  conn.run(q)
  #  q.run
  # (This is because by default, invoking the <b>+run+</b> instance method on a
  # query runs it on the most-recently-opened connection.)
  class Connection
    @@last = nil
    @@magic_number = 0xaf61ba35

    # Return the last opened connection, or throw if there is no such
    # connection.  Used by e.g. RQL_Query#run.
    def self.last
      return @@last if @@last
      raise RuntimeError, "No last connection.  Use RethinkDB::Connection.new."
    end

    def debug_socket # :nodoc:
      @socket;
    end

    def start_listener # :nodoc:
      @socket.send([@@magic_number].pack('L<'), 0)
      @listener = Thread.new do
        loop do
          response_length = @socket.recv(4).unpack('L<')[0]
          response = @socket.recv(response_length)
          #TODO: Recovery
          begin
            protob = Response.new.parse_from_string(response)
          rescue
            p response
            abort("Bad Protobuf.")
          end
          @mutex.synchronize do
            @data[protob.token] = protob
            if (@waiters[protob.token])
              cond = @waiters.delete protob.token
              cond.signal
            end
          end
        end
      end
    end

    def dispatch msg # :nodoc:
      PP.pp msg if $DEBUG
      payload = msg.serialize_to_string
      #File.open('payloads.txt', 'a') {|f| f.write(payload.inspect+"\n")}
      packet = [payload.length].pack('L<') + payload
      @socket.send(packet, 0)
      return msg.token
    end

    def wait token # :nodoc:
      @mutex.synchronize do
        (@waiters[token] = ConditionVariable.new).wait(@mutex) if not @data[token]
        return @data.delete token
      end
    end

    def continue token # :nodoc:
      msg = Query.new
      msg.type = Query::QueryType::CONTINUE
      msg.token = token
      dispatch msg
    end

    def error(protob,err=RuntimeError) # :nodoc:
      raise err,"RQL: #{protob.error_message}"
    end

    def token_iter(token) # :nodoc:
      done = false
      data = []
      loop do
        if (data == [])
          break if done
          protob = wait token
          case protob.status_code
          when Response::StatusCode::SUCCESS_JSON then
            yield JSON.parse('['+protob.response[0]+']')[0]
            return false
          when Response::StatusCode::SUCCESS_PARTIAL then
            continue token
            data.push *protob.response
          when Response::StatusCode::SUCCESS_STREAM then
            data.push *protob.response
            done = true
          when Response::StatusCode::SUCCESS_EMPTY then
            return false
          when Response::StatusCode::BAD_QUERY then error protob,SyntaxError
          when Response::StatusCode::RUNTIME_ERROR then error protob,RuntimeError
          else error protob
          end
        end
        #yield JSON.parse("["+data.shift+"]")[0] if data != []
        yield JSON.parse('['+data.shift+']')[0] if data != []
        #yield data.shift if data != []
      end
      return true
    end

    # Create a new connection to <b>+host+</b> on port <b>+port+</b>.
    # You may also optionally provide a default database to use when
    # running queries over that connection.  Example:
    #   c = Connection.new('localhost', 12346, 'default_db')
    def initialize(host='localhost', port=12346, default_db=nil)
      @@last = self
      @default_db = default_db
      @socket = TCPSocket.open(host, port)
      @waiters = {}
      @data = {}
      @mutex = Mutex.new
      start_listener
    end

    # Change the default database of a connection.
    def use(new_default_db)
      @default_db = new_default_db
    end

    # Run a query over the connection.  If you run a query that returns a JSON
    # expression (e.g. a reduce), you get back that JSON expression.  If you run
    # a query that returns a stream of results (e.g. filtering a table), you get
    # back an enumerable object of class RethinkDB::Query_Results.
    #
    # <b>NOTE:</b> unlike most enumerably objects, you can only iterate over the
    # result once.  See RethinkDB::Query_Results for more details.
    def run (query)
      is_atomic = (query.kind_of?(JSON_Expression) ||
                   query.kind_of?(Meta_Query) ||
                   query.kind_of?(Write_Query))
      args = @default_db ? [:default_db, @default_db] : []
      protob = query.query(*args)
      if is_atomic
        a = []
        token_iter(dispatch protob){|row| a.push row} ? a : a[0]
      else
        return Query_Results.new(self, dispatch(protob))
      end
    end

    # Close the connection.
    def close
      @listener.terminate!
      @socket.close
    end
  end
end
