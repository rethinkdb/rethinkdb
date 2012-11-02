# Copyright 2010-2012 RethinkDB, all rights reserved.
require 'socket'
require 'thread'
require 'json'

module RethinkDB
  module Faux_Abort # :nodoc:
    class Abort # :nodoc:
    end
  end
  # The result of a calling Connection#run on a query that returns a stream.
  # This class is <b>+Enumerable+</b>.  You can find documentation on Enumerable
  # classes at http://ruby-doc.org/core-1.9.3/Enumerable.html .
  #
  # <b>NOTE:</b> unlike most enumerable objects, you can only iterate over Query
  # results once.  The results are fetched lazily from the server in chunks
  # of 1000.  If you need to access to values multiple times, use the <b>+to_a+</b>
  # instance method to get an Array, and then work with that.
  class Query_Results
    def out_of_date # :nodoc:
      @conn.conn_id != @conn_id
    end

    def inspect # :nodoc:
      state = @run ? "(exhausted)" : "(enumerable)"
      extra = out_of_date ? " (Connection #{@conn.inspect} reset!)" : ""
      "#<RethinkDB::Query_Results:#{self.object_id} #{state}#{extra}: #{@query.inspect}>"
    end

    def initialize(query, connection, token) # :nodoc:
      @query = query
      @run = false
      @conn_id = connection.conn_id
      @conn = connection
      @token = token
    end

    def each (&block) # :nodoc:
      raise RuntimeError, "Can only iterate over Query_Results once!" if @run
      raise RuntimeError, "Connection has been reset!" if out_of_date
      @conn.token_iter(@query, @token, &block)
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
    def inspect # :nodoc:
      properties = "(#{@host}:#{@port}) (Default Database: '#{@default_db}')"
      state = @listener ? "(listening)" : "(closed)"
      "#<RethinkDB::Connection:#{self.object_id} #{properties} #{state}>"
    end

    @@last = nil
    @@magic_number = 0xaf61ba35

    # Return the last opened connection, or throw if there is no such
    # connection.  Used by e.g. RQL_Query#run.
    def self.last_connection
      return @@last if @@last
      raise RuntimeError, "No last connection.  Use RethinkDB::Connection.new."
    end

    def debug_socket # :nodoc:
      @socket;
    end

    def start_listener # :nodoc:
      class << @socket
        def read_exn(len) # :nodoc:
          buf = read len
          raise RuntimeError,"Connection closed by server." if !buf or buf.length != len
          return buf
        end
      end
      @socket.send([@@magic_number].pack('L<'), 0)
      @listener.terminate! if @listener
      @listener = Thread.new do
        loop do
          begin
            response_length = @socket.read_exn(4).unpack('L<')[0]
            response = @socket.read_exn(response_length)
          rescue RuntimeError => e
            @mutex.synchronize do
              @listener = nil
              @waiters.each {|kv| kv[1].signal}
            end
            Thread.current.terminate!
            abort("unreachable")
          end
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

    # Reconnect to the server.  This will interrupt all queries on the
    # server and invalidate all outstanding enumerables on the client.
    def reconnect
      @socket.close if @socket
      @socket = TCPSocket.open(@host, @port)
      @waiters = {}
      @data = {}
      @mutex = Mutex.new
      @conn_id += 1
      start_listener
      self
    end

    def dispatch msg # :nodoc:
      PP.pp msg if $DEBUG
      payload = msg.serialize_to_string
      #File.open('sexp_payloads.txt', 'a') {|f| f.write(payload.inspect+"\n")}
      packet = [payload.length].pack('L<') + payload
      @socket.send(packet, 0)
      return msg.token
    end

    def wait token # :nodoc:
      res = nil
      raise RuntimeError, "Connection closed by server!" if not @listener
      @mutex.synchronize do
        (@waiters[token] = ConditionVariable.new).wait(@mutex) if not @data[token]
        res = @data.delete token if @data[token]
      end
      raise RuntimeError, "Connection closed by server!" if !@listener or !res
      return res
    end

    def continue token # :nodoc:
      msg = Query.new
      msg.type = Query::QueryType::CONTINUE
      msg.token = token
      dispatch msg
    end

    def error(query,protob,err=RuntimeError) # :nodoc:
      bt = protob.backtrace ? protob.backtrace.frame : []
      #PP.pp bt
      msg = "RQL: #{protob.error_message}\n" + query.print_backtrace(bt)
      raise err,msg
    end

    def token_iter(query, token) # :nodoc:
      done = false
      data = []
      loop do
        if (data == [])
          break if done

          begin
            protob = wait token
          rescue @abort_module::Abort => e
            print "\nAborting query and reconnecting...\n"
            self.reconnect()
            raise e
          end

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
          when Response::StatusCode::BAD_QUERY then error query,protob,ArgumentError
          when Response::StatusCode::RUNTIME_ERROR then error query,protob,RuntimeError
          else error query,protob
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
    #   c = Connection.new('localhost', 28015, 'default_db')
    def initialize(host='localhost', port=28015, default_db='test')
      begin
        @abort_module = ::IRB
      rescue NameError => e
        @abort_module = Faux_Abort
      end
      @@last = self
      @host = host
      @port = port
      @default_db = default_db
      @conn_id = 0
      reconnect
    end
    attr_reader :default_db, :conn_id

    # Change the default database of a connection.
    def use(new_default_db)
      @default_db = new_default_db
    end

    # Run a query over the connection.  If you run a query that returns a JSON
    # expression (e.g. a reduce), you get back that JSON expression.  If you run
    # a query that returns a stream of results (e.g. filtering a table), you get
    # back an enumerable object of class RethinkDB::Query_Results.
    #
    # You may also provide some options as an optional second
    # argument.  The only useful option right now is
    # <b>+:use_outdated+</b>, which specifies whether tables can use
    # outdated information.  (If you specified this on a per-table
    # basis, specifying it again here won't override the original
    # choice.)
    #
    # <b>NOTE:</b> unlike most enumerably objects, you can only iterate over the
    # result once.  See RethinkDB::Query_Results for more details.
    def run (query, opts={})
      #File.open('sexp.txt', 'a') {|f| f.write(query.sexp.inspect+"\n")}
      is_atomic = (query.kind_of?(JSON_Expression) ||
                   query.kind_of?(Meta_Query) ||
                   query.kind_of?(Write_Query))
      map = {}
      map[:default_db] = @default_db if @default_db
      map[S.conn_outdated] = !!opts[:use_outdated]
      protob = query.query(map)
      if is_atomic
        a = []
        token_iter(query, dispatch(protob)){|row| a.push row} ? a : a[0]
      else
        return Query_Results.new(query, self, dispatch(protob))
      end
    end

    # Close the connection.
    def close
      @listener.terminate! if @listener
      @listener = nil
      @socket.close
      @socket = nil
    end
  end
end
