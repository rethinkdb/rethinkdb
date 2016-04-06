require 'monitor'
require 'set'
require 'socket'
require 'thread'
require 'timeout'
require 'pp' # This is needed for pretty_inspect
require 'openssl'
require 'base64'

module RethinkDB
  module Faux_Abort
    class Abort
    end
  end

  class EM_Guard
    @@mutex = Mutex.new
    @@registered = false
    @@conns = Set.new
    def self.register(conn)
      @@mutex.synchronize {
        if !@@registered
          @@registered = true
          EM.add_shutdown_hook {
            EM_Guard.remove_em_waiters
          }
        end
        @@conns += [conn]
      }
    end
    def self.unregister(conn)
      @@mutex.synchronize {
        @@conns -= [conn]
      }
    end
    def self.remove_em_waiters
      old_conns = Set.new
      @@mutex.synchronize {
        @@registered = false
        @@conns, old_conns = old_conns, @@conns
      }
      # This function acquires `@mon` on the connections, so it's
      # safer to do this outside our own synchronization.
      old_conns.each {|conn|
        conn.remove_em_waiters
      }
    end
  end

  class Handler
    def initialize
      @stopped = false
    end
    def handle(m, args, caller)
      if !stopped?
        mar = method(m).arity
        if mar == args.size
          send(m, *args)
        elsif mar == 0
          send(m)
        else
          send(m, *args, caller)
        end
      end
    end

    def on_open(caller)
    end
    def on_close(caller)
    end
    def on_wait_complete(caller)
    end

    def on_error(err, caller)
      raise err
    end
    def on_val(val, caller)
    end
    def on_array(arr, caller)
      if method(:on_atom).owner != Handler
        handle(:on_atom, [arr], caller)
      else
        arr.each {|x|
          break if stopped?
          handle(:on_stream_val, [x], caller)
        }
      end
    end
    def on_atom(val, caller)
      handle(:on_val, [val], caller)
    end
    def on_stream_val(val, caller)
      handle(:on_val, [val], caller)
    end

    def on_unhandled_change(val, caller)
      handle(:on_stream_val, [val], caller)
    end

    def stop
      @stopped = true
    end
    def stopped?
      @stopped
    end
  end

  class CallbackHandler < Handler
    def initialize(callback)
      if callback.arity > 2 || callback.arity < -3
        raise ArgumentError, "Wrong number of arguments for callback (callback " +
          "accepts #{callback.arity} arguments, but it should accept 0, 1 or 2)."
      end
      @callback = callback
    end
    def do_call(err, val)
      if @callback.arity == 0
        raise err if err
        @callback.call
      elsif @callback.arity == 1
        raise err if err
        @callback.call(val)
      else
        @callback.call(err, val)
      end
    end
    def on_val(x)
      do_call(nil, x)
    end
    def on_error(err)
      do_call(err, nil)
    end
  end

  class QueryHandle
    def initialize(handler, msg, all_opts, token, conn)
      @handler = handler
      @msg = msg
      @all_opts = all_opts
      @token = token
      @conn = conn
      @opened = false
      @closed = false
    end
    def closed?
      @closed
    end
    def close
      if !@closed
        handle_close
        return @conn.stop(@token)
      end
      return false
    end
    def handle(m, *args)
      begin
        @handler.handle(m, args, self)
      rescue Exception => e
        @handler.stop
        raise e
      end
    end
    def handle_open
      if !@opened
        @opened = true
        handle(:on_open)
      end
    end
    def handle_close
      if !@closed
        @closed = true
        handle(:on_close)
      end
    end
    def handle_force_close
      if !@closed
        handle(:on_error, ReqlRuntimeError.new("Connection is closed."))
      end
      handle_close
    end
    def safe_next_tick(&b)
      EM.next_tick {
        b.call if !@closed
      }
    end
    def callback(res)
      begin
        if @handler.stopped? || !EM.reactor_running?
          @closed = true
          @conn.stop(@token)
          return
        elsif res
          is_cfeed = (res['n'] & [Response::ResponseNote::SEQUENCE_FEED,
                                  Response::ResponseNote::ATOM_FEED,
                                  Response::ResponseNote::ORDER_BY_LIMIT_FEED,
                                  Response::ResponseNote::UNIONED_FEED]) != []
          if (res['t'] == Response::ResponseType::SUCCESS_PARTIAL) ||
              (res['t'] == Response::ResponseType::SUCCESS_SEQUENCE)
            safe_next_tick {
              handle_open
              if res['t'] == Response::ResponseType::SUCCESS_PARTIAL
                @conn.register_query(@token, @all_opts, self) if !@conn.closed?
                @conn.dispatch([Query::QueryType::CONTINUE], @token) if !@conn.closed?
              end
              Shim.response_to_native(res, @msg, @all_opts).each {|row|
                if is_cfeed
                  if (row.has_key?('new_val') && row.has_key?('old_val') &&
                      @handler.respond_to?(:on_change))
                    handle(:on_change, row['old_val'], row['new_val'])
                  elsif (row.has_key?('new_val') && !row.has_key?('old_val') &&
                         @handler.respond_to?(:on_initial_val))
                    handle(:on_initial_val, row['new_val'])
                  elsif (row.has_key?('old_val') && !row.has_key?('new_val') &&
                         @handler.respond_to?(:on_uninitial_val))
                    handle(:on_uninitial_val, row['old_val'])
                  elsif row.has_key?('error') && @handler.respond_to?(:on_change_error)
                    handle(:on_change_error, row['error'])
                  elsif row.has_key?('state') && @handler.respond_to?(:on_state)
                    handle(:on_state, row['state'])
                  else
                    handle(:on_unhandled_change, row)
                  end
                else
                  handle(:on_stream_val, row)
                end
              }
              if (res['t'] == Response::ResponseType::SUCCESS_SEQUENCE ||
                  @conn.closed?)
                handle_close
              end
            }
          elsif res['t'] == Response::ResponseType::SUCCESS_ATOM
            safe_next_tick {
              return if @closed
              handle_open
              val = Shim.response_to_native(res, @msg, @all_opts)
              if val.is_a?(Array)
                handle(:on_array, val)
              else
                handle(:on_atom, val)
              end
              handle_close
            }
          elsif res['t'] == Response::ResponseType::WAIT_COMPLETE
            safe_next_tick {
              return if @closed
              handle_open
              handle(:on_wait_complete)
              handle_close
            }
          else
            exc = nil
            begin
              exc = Shim.response_to_native(res, @msg, @all_opts)
            rescue Exception => e
              exc = e
            end
            safe_next_tick {
              return if @closed
              handle_open
              handle(:on_error, e)
              handle_close
            }
          end
        else
          safe_next_tick {
            return if @closed
            handle_close
          }
        end
      rescue Exception => e
        safe_next_tick {
          return if @closed
          handle_open
          handle(:on_error, e)
          handle_close
        }
      end
    end
  end

  class RQL
    @@default_conn = nil
    def self.set_default_conn c; @@default_conn = c; end
    def parse(*args, &b)
      conn = nil
      opts = nil
      block = nil
      args = args.map{|x| x.is_a?(Class) ? x.new : x}
      args.each {|arg|
        case arg
        when RethinkDB::Connection
          raise ArgumentError, "Unexpected second Connection #{arg.inspect}." if conn
          conn = arg
        when Hash
          raise ArgumentError, "Unexpected second Hash #{arg.inspect}." if opts
          opts = arg
        when Proc
          raise ArgumentError, "Unexpected second callback #{arg.inspect}." if block
          block = arg
        when Handler
          raise ArgumentError, "Unexpected second callback #{arg.inspect}." if block
          block = arg
        else
          raise ArgumentError, "Unexpected argument #{arg.inspect} " +
            "(got #{args.inspect})."
        end
      }
      conn = @@default_conn if !conn
      opts = {} if !opts
      block = b if !block
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
      if !conn
        raise ArgumentError, "No connection specified!\n" \
        "Use `query.run(conn)` or `conn.repl(); query.run`."
      end
      {conn: conn, opts: opts, block: block}
    end
    def run(*args, &b)
      unbound_if(@body == RQL)
      args = parse(*args, &b)
      if args[:block].is_a?(Handler)
        raise RuntimeError, "Cannot call `run` with a handler, did you mean `em_run`?"
      end
      args[:conn].run(@body, args[:opts], args[:block])
    end
    def em_run(*args, &b)
      if !EM.reactor_running?
        raise RuntimeError, "RethinkDB::RQL::em_run can only be called inside `EM.run`"
      end
      unbound_if(@body == RQL)
      args = parse(*args, &b)
      if args[:block].is_a?(Proc)
        args[:block] = CallbackHandler.new(args[:block])
      end
      if !args[:block].is_a?(Handler)
        raise ArgumentError, "No handler specified."
      end

      # If the user has defined the `on_state` method, we assume they want states.
      if args[:block].respond_to?(:on_state)
        args[:opts] = args[:opts].merge(include_states: true)
      end

      EM_Guard.register(args[:conn])
      args[:conn].run(@body, args[:opts], args[:block])
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
      "#<RethinkDB::Cursor:#{object_id} #{state}#{extra}: #{RPP.pp(@msg)}" +
        (@run ? "" : "\n#{preview}") + ">"
    end

    def initialize(results, msg, connection, opts, token, more) # :nodoc:
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
      raise ReqlRuntimeError, "Can only iterate over a cursor once." if @run
      return enum_for(:each) if !block
      @run = true
      while true
        @results.each(&block)
        return self if !@more
        raise ReqlRuntimeError, "Connection is closed." if @more && out_of_date
        wait_for_batch(nil)
      end
    end

    def close
      if @more
        @more = false
        @conn.stop(@token)
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
      if @run
        raise ReqlRuntimeError, "Cannot call `next` on a cursor after calling `each`."
      end
      if @more && out_of_date
        raise ReqlRuntimeError, "Connection is closed."
      end
      timeout = wait
      if wait == true
        timeout = nil
      elsif !wait
        timeout = 0
      end

      while @results.length == 0
        raise StopIteration if !@more
        wait_for_batch(timeout)
      end

      @results.shift
    end
  end

  class Connection
    include OpenSSL

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

      opts = Hash[opts.map{|(k,v)| [k.to_sym,v]}] if opts.is_a?(Hash)
      opts = {:host => opts} if opts.is_a?(String)
      @host = opts[:host] || "localhost"
      @port = (opts[:port] || 28015).to_i
      @default_db = opts[:db]
      @user = opts[:user] || "admin"
      @user = @user.gsub("=", "=3D").gsub(",","=2C")
      if opts[:password] && opts[:auth_key]
        raise ReqlDriverError, "Cannot specify both a password and an auth key."
      end
      @password = opts[:password] || opts[:auth_key] || ""
      @nonce = SecureRandom.base64(18)
      @timeout = opts[:timeout].to_i
      @timeout = 20 if @timeout <= 0
      @ssl_opts = opts[:ssl] || {}

      @@last = self
      @default_opts = @default_db ? {:db => RQL.new.db(@default_db)} : {}
      @conn_id = 0

      @token_cnt = 0
      @token_cnt_mutex = Mutex.new

      connect()
    end
    attr_reader :host, :port, :default_db, :conn_id

    def client_port
      is_open() ? @socket.addr[1] : nil
    end
    def client_address
      is_open() ? @socket.addr[3] : nil
    end

    def new_token
      @token_cnt_mutex.synchronize{@token_cnt += 1}
    end

    def register_query(token, opts, callback=nil)
      if !opts[:noreply]
        @mon.synchronize {
          if @waiters.has_key?(token)
            raise ReqlDriverError, "Internal driver error, token already in use."
          end
          @waiters[token] = callback ? callback : @mon.new_cond
          @opts[token] = opts
        }
      end
    end
    def run_internal(q, opts, token)
      register_query(token, opts)
      dispatch(q, token)
      opts[:noreply] ? nil : wait(token, nil)
    end
    def stop(token)
      dispatch([Query::QueryType::STOP], token)
      @mon.synchronize {
        !!@waiters.delete(token)
      }
    end
    def run(msg, opts, b)
      reconnect(:noreply_wait => false) if @auto_reconnect && !is_open()
      raise ReqlRuntimeError, "Connection is closed." if !is_open()

      global_optargs = {}
      all_opts = @default_opts.merge(opts)
      if all_opts.keys.include?(:noreply)
        all_opts[:noreply] = !!all_opts[:noreply]
      end

      token = new_token
      q = [Query::QueryType::START,
           msg,
           Hash[all_opts.map {|k,v|
                  [k.to_s, (v.is_a?(RQL) ? v.to_pb : RQL.new.expr(v).to_pb)]
                }]]

      if b.is_a? Handler
        callback = QueryHandle.new(b, msg, all_opts, token, self)
        register_query(token, all_opts, callback)
        dispatch(q, token)
        return callback
      else
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
            value.close if value.is_a?(Cursor)
          end
        else
          real_val
        end
      end
    end

    def send packet
      @mon.synchronize {
        written = 0
        while written < packet.length
          # Supposedly slice will not copy the array if it goes all
          # the way to the end We use IO::syswrite here rather than
          # IO::write because of incompatibilities in JRuby regarding
          # filling up the TCP send buffer.  Reference:
          # https://github.com/rethinkdb/rethinkdb/issues/3795
          written += @socket.syswrite(packet.slice(written, packet.length))
        end
      }
    end

    def dispatch(msg, token)
      payload = Shim.dump_json(msg).force_encoding('BINARY')
      prefix = [token, payload.bytesize].pack('Q<L<')
      send(prefix + payload)
      return token
    end

    def wait(token, timeout)
      begin
        @mon.synchronize {
          end_time = timeout ? Time.now.to_f + timeout : nil
          loop {
            res = @data.delete(token)
            return res if res

            # Theoretically we only need to check the second property,
            # but this is safer in case someone makes changes to
            # `close` in the future.
            if !is_open() || !@waiters.has_key?(token)
              raise ReqlRuntimeError, "Connection is closed."
            end

            if end_time
              cur_time = Time.now.to_f
              if cur_time >= end_time
                raise Timeout::Error, "Timed out waiting for cursor response."
              else
                # We can't use `wait_while` because it doesn't take a
                # timeout, and we can't use an external `timeout {
                # ... }` block because in Ruby 1.9.1 it seems to confuse
                # the synchronization in `@mon` to be timed out while
                # waiting in a synchronize block.
                @waiters[token].wait(end_time - cur_time)
              end
            else
              @waiters[token].wait
            end
          }
        }
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
      state = is_open() ? "(open)" : "(closed)"
      "#<RethinkDB::Connection:#{object_id} #{properties} #{state}>"
    end

    @@last = nil
    @@magic_number = VersionDummy::Version::V1_0
    @@protocol_version = 0
    @@wire_protocol = VersionDummy::Protocol::JSON

    def debug_socket; @socket; end

    # Reconnect to the server.  This will interrupt all queries on the
    # server (if :noreply_wait => false) and invalidate all outstanding
    # enumerables on the client.
    def reconnect(opts={})
      raise ArgumentError, "Argument to reconnect must be a hash." if opts.class != Hash
      close(opts)
      connect()
    end

    def connect()
      raise RuntimeError, "Connection must be closed before calling connect." if @socket
      init_socket
      @mon = Monitor.new
      @waiters = {}
      @opts = {}
      @data = {}
      @conn_id += 1
      start_listener
      self
    end

    def init_socket
      unless @ssl_opts.empty?
        @tcp_socket = base_socket
        context = create_context(@ssl_opts)
        @socket = OpenSSL::SSL::SSLSocket.new(@tcp_socket, context)
        @socket.sync_close = true
        @socket.connect
        verify_cert!(@socket, context)
      else
        @socket = base_socket
      end
    end

    def base_socket
      socket = TCPSocket.open(@host, @port)
      socket.setsockopt(Socket::IPPROTO_TCP, Socket::TCP_NODELAY, 1)
      socket.setsockopt(Socket::SOL_SOCKET, Socket::SO_KEEPALIVE, 1)
      socket
    end

    def create_context(options)
      context = OpenSSL::SSL::SSLContext.new
      context.ssl_version = :TLSv1_2
      if options[:ca_certs]
        context.ca_file = options[:ca_certs]
        context.verify_mode = OpenSSL::SSL::VERIFY_PEER
      else
        raise 'ssl options provided but missing required "ca_certs" option'
      end
      context
    end

    def verify_cert!(socket, context)
      if context.verify_mode == OpenSSL::SSL::VERIFY_PEER
        unless OpenSSL::SSL.verify_certificate_identity(socket.peer_cert, host)
          raise 'SSL handshake failed due to a hostname mismatch.'
        end
      end
    end

    def is_open
      @socket && @listener
    end
    def closed?
      !is_open
    end

    def close(opts={})
      EM_Guard.unregister(self)
      raise ArgumentError, "Argument to close must be a hash." if opts.class != Hash
      if !(opts.keys - [:noreply_wait]).empty?
        raise ArgumentError, "close does not understand these options: " +
          (opts.keys - [:noreply_wait]).to_s
      end
      opts[:noreply_wait] = true if !opts.keys.include?(:noreply_wait)

      @mon.synchronize {
        @opts.clear
        @data.clear
        @waiters.each {|k,v|
          case v
          when QueryHandle
            v.handle_force_close
          when MonitorMixin::ConditionVariable
            @waiters[k] = nil
            v.signal
          end
        }
        @waiters.clear
      }

      noreply_wait() if opts[:noreply_wait] && is_open()
      if @listener
        @listener.terminate
        @listener.join
      end
      @socket.close if @socket
      @listener = nil
      @socket = nil
      self
    end

    def noreply_wait
      raise ReqlRuntimeError, "Connection is closed." if !is_open()
      q = [Query::QueryType::NOREPLY_WAIT]
      res = run_internal(q, {noreply: false}, new_token)
      if res['t'] != Response::ResponseType::WAIT_COMPLETE
        raise ReqlRuntimeError, "Unexpected response to noreply_wait: " + PP.pp(res, "")
      end
      nil
    end

    def server
      raise ReqlRuntimeError, "Connection is closed." if !is_open()
      q = [Query::QueryType::SERVER_INFO]
      res = run_internal(q, {noreply: false}, new_token)
      if res['t'] != Response::ResponseType::SERVER_INFO
        raise ReqlRuntimeError, "Unexpected response to server_info: " + PP.pp(res, "")
      end
      res['r'][0]
    end

    def self.last
      return @@last if @@last
      raise ReqlRuntimeError, "No last connection.  Use RethinkDB::Connection.new."
    end

    def remove_em_waiters
      @mon.synchronize {
        @waiters.each {|k,v|
          if v.is_a? QueryHandle
            v.handle_force_close
            @waiters.delete(k)
          end
        }
      }
    end

    def note_data(token, data) # Synchronize around this!
      @opts.delete(token)
      w = @waiters.delete(token)
      case w
      when MonitorMixin::ConditionVariable
        @data[token] = data
        w.signal
      when QueryHandle
        w.callback(data)
      when nil
        # nothing
      else
        raise ReqlDriverError, "Unrecognized value #{w.inspect} in `@waiters`."
      end
    end

    def note_error(token, e) # Synchronize around this!
      data = {
        't' => Response::ResponseType::CLIENT_ERROR,
        'r' => [e.message],
        'b' => []
      }
      note_data(token, data)
    end

    def rcv_json
      response = ""
      while response[-1..-1] != "\0"
        response += @socket.read_exn(1, @timeout)
      end
      begin
        res = JSON.parse(response[0...-1])
        if !res['success']
          msg = "Handshake error (#{res})."
          ecode = res['error_code']
          if ecode && ecode >= 10 && ecode <= 20
            msg = res['error'] if res['error'].to_s != ""
            raise ReqlAuthError, msg
          else
            raise ReqlDriverError, msg
          end
        end
      rescue Exception => e
        if !e.class.ancestors.include?(ReqlDriverError)
          raise ReqlDriverError, "Connection closed by server (#{e})."
        else
          raise e
        end
      end
      return res
    end

    def send_json(x)
      send(x.to_json + "\0")
    end

    def check_version(server_info)
      if server_info['min_protocol_version'] > @@protocol_version ||
          server_info['max_protocol_version'] < @@protocol_version
        raise ReqlDriverError, "Version mismatch: Driver uses #{@@protocol_version} "+
          "but server accepts "+
          "[#{server_info['min_version']}, #{server_info['max_version']}]."
      end
    end

    def check_nonce(server_nonce, nonce)
      if !server_nonce.start_with?(nonce)
        raise ReqlDriverError, "Invalid nonce #{server_nonce} received from server."
      end
    end

    @@fast_auth_func = lambda {|*args|
      digest = OpenSSL::Digest::SHA256.new
      OpenSSL::PKCS5.pbkdf2_hmac(*args, digest.digest_length, digest)
    }
    @@slow_auth_func = lambda {|password, salt, iter|
      mac = OpenSSL::HMAC.new(password, OpenSSL::Digest::SHA256.new)
      acc = t = mac.update(salt + "\0\0\0\1").digest
      mac.reset
      (iter-1).times{acc = xor(acc, t = mac.update(t).digest); mac.reset}
      res = acc
    }
    @@auth_func = @@fast_auth_func
    @@auth_func_mutex = Mutex.new

    @@auth_cache = {}
    @@auth_cache_mutex = Mutex.new

    def pbkdf2_hmac_sha256(*args)
      auth_func = res = nil
      @@auth_cache_mutex.synchronize {
        res = @@auth_cache[args]
        return res if res
      }

      @@auth_func_mutex.synchronize {
        auth_func = @@auth_func
      }
      begin
        res = auth_func.call(*args)
      rescue NotImplementedError => e
        if auth_func != @@slow_auth_func
          @@auth_func_mutex.synchronize {
            auth_func = @@auth_func = @@slow_auth_func
          }
          res = auth_func.call(*args)
        else
          raise e
        end
      end

      @@auth_cache_mutex.synchronize {
        @@auth_cache[args] = res
      }
      return res
    end

    def hmac(*args)
      OpenSSL::HMAC.digest(OpenSSL::Digest::SHA256.new, *args)
    end

    def sha256(str)
      OpenSSL::Digest.digest("SHA256", str)
    end

    def self.xor(str1, str2)
      out = str1.dup
      out.bytesize.times {|i|
        out.setbyte(i, out.getbyte(i) ^ str2.getbyte(i))
      }
      return out
    end

    def const_eq(a, b)
      return false if a.size != b.size
      residue = 0
      a.unpack('C*').zip(b.unpack('C*')).each{|x,y|
        residue |= (x ^ y)
      }
      return residue == 0
    end

    def check_server_signature_claim(server_signature_claim, server_signature)
      if !const_eq(server_signature_claim, server_signature)
        sig1 = Base64.strict_encode64(server_signature_claim)
        sig2 = Base64.strict_encode64(server_signature)
        raise ReqlAuthError, "Server signature #{sig1} does "+
          "not match expected signature #{sig2}."
      end
    end

    def do_handshake
      begin
        send([@@magic_number].pack('L<'))
        client_first_message_bare = "n=#{@user},r=#{@nonce}"
        send_json({ protocol_version: @@protocol_version,
                    authentication_method: "SCRAM-SHA-256",
                    authentication: "n,,#{client_first_message_bare}" })

        server_version_msg = rcv_json
        check_version(server_version_msg)

        auth_resp = rcv_json
        server_first_message = auth_resp['authentication']

        fields = Hash[server_first_message.split(',').map{|x| x.split('=', 2)}]
        server_nonce = fields['r']

        client_final_message_without_proof = "c=biws,r=#{server_nonce}"
        auth_message = "#{client_first_message_bare},#{server_first_message},"+
          client_final_message_without_proof
        check_nonce(server_nonce, @nonce)
        salt = Base64.decode64(fields['s'])
        iter = fields['i'].to_i

        salted_password = pbkdf2_hmac_sha256(@password, salt, iter)

        client_key = hmac(salted_password, "Client Key")
        stored_key = sha256(client_key)
        client_signature = hmac(stored_key, auth_message)
        client_proof = Connection::xor(client_key, client_signature)
        cproof_64 = Base64.strict_encode64(client_proof)

        msg = "#{client_final_message_without_proof},p=#{cproof_64}"
        send_json({authentication: msg})

        sig_resp = rcv_json
        fields = Hash[sig_resp['authentication'].split(',').map{|x| x.split('=', 2)}]
        server_signature_claim = Base64::decode64(fields['v'])
        server_key = hmac(salted_password, "Server Key")
        server_signature = hmac(server_key, auth_message)
        check_server_signature_claim(server_signature_claim, server_signature)
      rescue ReqlError => e
        raise e
      rescue Exception => e
        raise ReqlDriverError, "Error during handshake: #{e.inspect} #{e.backtrace}"
      end
    end

    def start_listener
      class << @socket
        def maybe_timeout(sec=nil, &b)
          sec ? Timeout::timeout(sec, &b) : b.call
        end
        def read_exn(len, timeout_sec=nil)
          maybe_timeout(timeout_sec) {
            buf = read len
            if !buf || buf.length != len
              raise ReqlRuntimeError, "Connection closed by server."
            end
            return buf
          }
        end
      end
      do_handshake
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
              raise ReqlRuntimeError, "Bad response, server is buggy.\n" +
                "#{e.inspect}\n" + response
            end
            @mon.synchronize{note_data(token, data)}
          rescue Exception => e
            @mon.synchronize {
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
