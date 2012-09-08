require 'socket'
require 'thread'
require 'json'

#TODO: Make sure tokens don't overflow.

module RethinkDB
  class Query_Results
    include Enumerable
    def initialize(connection, token)
      @run = false
      @connection = connection
      @token = token
    end

    def each (&block)
      raise RuntimeError, "Can only iterate over Query_Results once!" if @run
      @connection.token_iter(@token, &block)
      @run = true
      return self
    end
  end

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
      payload = msg.serialize_to_string
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

    #TODO: doc
    def run (query)
      is_atomic = (query.kind_of?(JSON_Expression) ||
                   query.kind_of?(Meta_Query) ||
                   query.kind_of?(Write_Query))
      protob = query.query
      if is_atomic
        a = []
        token_iter(dispatch protob){|row| a.push row} ? a : a[0]
      else
        return Query_Results.new(self, dispatch(protob))
      end
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
