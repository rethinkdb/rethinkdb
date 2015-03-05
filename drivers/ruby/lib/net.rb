require 'socket'
require 'thread'
require 'timeout'
require 'pp' # This is needed for pretty_inspect

module RethinkDB
  module Faux_Abort
    class Abort
    end
  end

  class RQL
    @@default_conn = nil
    def self.set_default_conn c; @@default_conn = c; end
    def run(c=@@default_conn, opts=nil, &b)
      unbound_if(@body == RQL)
      c, opts = @@default_conn, c if !opts && !c.kind_of?(RethinkDB::Connection)
      opts = {} if !opts
      opts = {opts => true} if opts.class != Hash
      if (tf = opts[:time_format])
        opts[:time_format] = (tf = tf.to_s)
        if tf != 'raw' && tf != 'native'
          raise ArgumentError, "`time_format` must be 'raw' or 'native' (got `#{tf}`)."
        end
      end
      if (gf = opts[:group_format])
        opts[:group_format] = (gf = gf.to_s)
        if gf != 'raw' && gf != 'native'
          raise ArgumentError, "`group_format` must be 'raw' or 'native' (got `#{gf}`)."
        end
      end
      if (bf = opts[:binary_format])
        opts[:binary_format] = (bf = bf.to_s)
        if bf != 'raw' && bf != 'native'
          raise ArgumentError, "`binary_format` must be 'raw' or 'native' (got `#{bf}`)."
        end
      end
      if !c
        raise ArgumentError, "No connection specified!\n" \
        "Use `query.run(conn)` or `conn.repl(); query.run`."
      end
      c.run(@body, opts, &b)
    end
  end

  class Cursor
    include Enumerable
    def out_of_date # :nodoc:
      @conn.conn_id != @conn_id || !@conn.is_open()
    end

    def inspect # :nodoc:
      preview_res = @results[0...10]
      if @results.size > 10 || @more
        preview_res << (dots = "..."; class << dots; def inspect; "..."; end; end; dots)
      end
      preview = preview_res.pretty_inspect[0...-1]
      state = @run ? "(exhausted)" : "(enumerable)"
      extra = out_of_date ? " (Connection #{@conn.inspect} is closed.)" : ""
      "#<RethinkDB::Cursor:#{self.object_id} #{state}#{extra}: #{RPP.pp(@msg)}" +
        (@run ? "" : "\n#{preview}") + ">"
    end

    def initialize(results, msg, connection, opts, token, more = true) # :nodoc:
      @more = more
      @results = results
      @msg = msg
      @run = false
      @conn_id = connection.conn_id
      @conn = connection
      @opts = opts
      @token = token
      fetch_batch
    end

    def each(&block) # :nodoc:
      raise RqlRuntimeError, "Can only iterate over a cursor once." if @run
      return self.enum_for(:each) if !block
      @run = true
      while true
        @results.each(&block)
        return self if !@more
        raise RqlRuntimeError, "Connection is closed." if @more && out_of_date
        wait_for_batch(nil)
      end
    end

    def close
      if @more
        @more = false
        q = [Query::QueryType::STOP]
        @conn.run_internal(q, @opts.merge({noreply: true}), @token)
        return true
      end
      return false
    end

    def wait_for_batch(timeout)
        res = @conn.wait(@token, timeout)
        @results = Shim.response_to_native(res, @msg, @opts)
        if res['t'] == Response::ResponseType::SUCCESS_SEQUENCE
          @more = false
        else
          fetch_batch
        end
    end

    def fetch_batch
      if @more
        @conn.register_query(@token, @opts)
        @conn.dispatch([Query::QueryType::CONTINUE], @token)
      end
    end

    def next(wait=true)
      raise RqlRuntimeError, "Cannot call `next` on a cursor " +
                             "after calling `each`." if @run
      raise RqlRuntimeError, "Connection is closed." if @more && out_of_date
      timeout = wait == true ? nil : ((wait == false || wait.nil?) ? 0 : wait)

      while @results.length == 0
        raise StopIteration if !@more
        wait_for_batch(timeout)
      end

      @results.shift
    end
  end

  class Connection
    def auto_reconnect(x=true)
      @auto_reconnect = x
      self
    end
    def repl; RQL.set_default_conn self; end

    def initialize(opts={})
      begin
        @abort_module = ::IRB
      rescue NameError => e
        @abort_module = Faux_Abort
      end

      opts = {:host => opts} if opts.class == String
      @host = opts[:host] || "localhost"
      @port = opts[:port] || 28015
      @default_db = opts[:db]
      @auth_key = opts[:auth_key] || ""

      @@last = self
      @default_opts = @default_db ? {:db => RQL.new.db(@default_db)} : {}
      @conn_id = 0

      @token_cnt = 0
      @token_cnt_mutex = Mutex.new

      self.connect()
    end
    attr_reader :host, :port, :default_db, :conn_id

    def new_token
      @token_cnt_mutex.synchronize{@token_cnt += 1}
    end

    def register_query(token, opts)
      if !opts[:noreply]
        @listener_mutex.synchronize{
          raise RqlDriverError, "Internal driver error, token already in use." \
            if @waiters.has_key?(token)
          @waiters[token] = ConditionVariable.new
          @opts[token] = opts
        }
      end
    end
    def run_internal(q, opts, token)
      register_query(token, opts)
      dispatch(q, token)
      opts[:noreply] ? nil : wait(token, nil)
    end
    def run(msg, opts, &b)
      reconnect(:noreply_wait => false) if @auto_reconnect && !self.is_open()
      raise RqlRuntimeError, "Connection is closed." if !self.is_open()

      global_optargs = {}
      all_opts = @default_opts.merge(opts)
      if all_opts.keys.include?(:noreply)
        all_opts[:noreply] = !!all_opts[:noreply]
      end

      token = new_token
      q = [Query::QueryType::START,
           msg,
           Hash[all_opts.map {|k,v|
                  [k.to_s, (v.class == RQL ? v.to_pb : RQL.new.expr(v).to_pb)]
                }]]

      res = run_internal(q, all_opts, token)
      return res if !res
      if res['t'] == Response::ResponseType::SUCCESS_PARTIAL
        value = Cursor.new(Shim.response_to_native(res, msg, opts),
                           msg, self, opts, token, true)
      elsif res['t'] == Response::ResponseType::SUCCESS_SEQUENCE
        value = Cursor.new(Shim.response_to_native(res, msg, opts),
                   msg, self, opts, token, false)
      else
        value = Shim.response_to_native(res, msg, opts)
      end

      if res['p']
        real_val = {
          "profile" => res['p'],
          "value" => value
        }
      else
        real_val = value
      end

      if b
        begin
          b.call(real_val)
        ensure
          value.close if value.class == Cursor
        end
      else
        real_val
      end
    end

    def send packet
      written = 0
      while written < packet.length
        # Supposedly slice will not copy the array if it goes all the way to the end
        # We use IO::syswrite here rather than IO::write because of incompatibilities in
        # JRuby regarding filling up the TCP send buffer.
        # Reference: https://github.com/rethinkdb/rethinkdb/issues/3795
        written += @socket.syswrite(packet.slice(written, packet.length))
      end
    end

    def dispatch(msg, token)
      payload = Shim.dump_json(msg).force_encoding('BINARY')
      prefix = [token, payload.bytesize].pack('Q<L<')
      send(prefix + payload)
      return token
    end

    def wait(token, timeout)
      begin
        res = nil
        @listener_mutex.synchronize {
          raise RqlRuntimeError, "Connection is closed." if !@waiters.has_key?(token)
          res = @data.delete(token)
          if res.nil?
            @waiters[token].wait(@listener_mutex, timeout)
            res = @data.delete(token)
            if res.nil?
              raise Timeout::Error, "Timed out waiting for cursor response."
            end
          end
          @waiters.delete(token)
        }
        raise RqlRuntimeError, "Connection is closed." if res.nil? && !self.is_open()
        raise RqlDriverError, "Internal driver error, no response found." if res.nil?
        return res
      rescue @abort_module::Abort => e
        print "\nAborting query and reconnecting...\n"
        reconnect(:noreply_wait => false)
        raise e
      end
    end

    # Change the default database of a connection.
    def use(new_default_db)
      @default_db = new_default_db
      @default_opts[:db] = RQL.new.db(new_default_db)
    end

    def inspect
      db = @default_opts[:db] || RQL.new.db('test')
      properties = "(#{@host}:#{@port}) (Default DB: #{db.inspect})"
      state = self.is_open() ? "(open)" : "(closed)"
      "#<RethinkDB::Connection:#{self.object_id} #{properties} #{state}>"
    end

    @@last = nil
    @@magic_number = VersionDummy::Version::V0_4
    @@wire_protocol = VersionDummy::Protocol::JSON

    def debug_socket; @socket; end

    # Reconnect to the server.  This will interrupt all queries on the
    # server (if :noreply_wait => false) and invalidate all outstanding
    # enumerables on the client.
    def reconnect(opts={})
      raise ArgumentError, "Argument to reconnect must be a hash." if opts.class != Hash
      self.close(opts)
      self.connect()
    end

    def connect()
      raise RuntimeError, "Connection must be closed before calling connect." if @socket
      @socket = TCPSocket.open(@host, @port)
      @listener_mutex = Mutex.new
      @waiters = {}
      @opts = {}
      @data = {}
      @conn_id += 1
      start_listener
      self
    end

    def is_open()
      @socket && @listener
    end

    def close(opts={})
      raise ArgumentError, "Argument to close must be a hash." if opts.class != Hash
      if !(opts.keys - [:noreply_wait]).empty?
        raise ArgumentError, "close does not understand these options: " +
          (opts.keys - [:noreply_wait]).to_s
      end
      opts[:noreply_wait] = true if !opts.keys.include?(:noreply_wait)

      self.noreply_wait() if opts[:noreply_wait] && self.is_open()
      if @listener
        @listener.terminate
        @listener.join
      end
      @socket.close if @socket
      @listener = nil
      @socket = nil
      @listener_mutex.synchronize {
        @opts.clear
        @data.clear
        @waiters.values.each{ |w| w.signal }
        @waiters.clear
      }
      self
    end

    def noreply_wait
      raise RqlRuntimeError, "Connection is closed." if !self.is_open()
      q = [Query::QueryType::NOREPLY_WAIT]
      res = run_internal(q, {noreply: false}, new_token)
      if res['t'] != Response::ResponseType::WAIT_COMPLETE
        raise RqlRuntimeError, "Unexpected response to noreply_wait: " + PP.pp(res, "")
      end
      nil
    end

    def self.last
      return @@last if @@last
      raise RqlRuntimeError, "No last connection.  Use RethinkDB::Connection.new."
    end

    def note_data(token, data) # Synchronize around this!
      raise RqlDriverError, "Unknown token in response." if !@waiters.has_key?(token)
      @data[token] = data
      @opts.delete(token)
      w = @waiters[token]
      w.signal if w
    end

    def note_error(token, e) # Synchronize around this!
      data = {
        't' => Response::ResponseType::CLIENT_ERROR,
        'r' => [e.message],
        'b' => []
      }
      note_data(token, data)
    end

    def start_listener
      class << @socket
        def maybe_timeout(sec=nil, &b)
          sec ? timeout(sec, &b) : b.call
        end
        def read_exn(len, timeout_sec=nil)
          maybe_timeout(timeout_sec) {
            buf = read len
            if !buf || buf.length != len
              raise RqlRuntimeError, "Connection closed by server."
            end
            return buf
          }
        end
      end
      send([@@magic_number, @auth_key.size].pack('L<L<') +
            @auth_key + [@@wire_protocol].pack('L<'))
      response = ""
      while response[-1..-1] != "\0"
        response += @socket.read_exn(1, 20)
      end
      response = response[0...-1]
      if response != "SUCCESS"
        raise RqlRuntimeError, "Server dropped connection with message: \"#{response}\""
      end

      raise RqlDriverError, "Internal driver error, listener already started." \
        if @listener
      @listener = Thread.new {
        while true
          begin
            token = nil
            token = @socket.read_exn(8).unpack('q<')[0]
            response_length = @socket.read_exn(4).unpack('L<')[0]
            response = @socket.read_exn(response_length)
            begin
              data = Shim.load_json(response, @opts[token])
            rescue Exception => e
              raise RqlRuntimeError, "Bad response, server is buggy.\n" +
                "#{e.inspect}\n" + response
            end
            @listener_mutex.synchronize{note_data(token, data)}
          rescue Exception => e
            @listener_mutex.synchronize {
              @waiters.keys.each{ |k| note_error(k, e) }
              @listener = nil
              Thread.current.terminate
              abort("unreachable")
            }
          end
        end
      }
    end
  end
end
