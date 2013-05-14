#!/usr/bin/ruby
require 'rubygems'
require 'socket'
require 'pp'
require 'prettyprint'
require 'socket'
require 'thread'
require 'json'

require 'protobuf/message/field'
if 2**63 != 9223372036854775808
  $stderr.puts "WARNING: Ruby believes 2**63 = #{2**63} rather than 9223372036854775808!"
  $stderr.puts "Consider upgrading your verison of Ruby."
  $stderr.puts "Hot-patching ruby_protobuf to compensate..."
  rethinkdb_verbose, $VERBOSE = $VERBOSE, nil
  module Protobuf
    module Field
      class VarintField < BaseField
        INT64_MAX = 9223372036854775807
        INT64_MIN = -9223372036854775808
        UINT64_MAX = 18446744073709551615
      end
    end
  end
  $VERBOSE = rethinkdb_verbose
end

module Query_Language_1
  # Copyright 2010-2012 RethinkDB, all rights reserved.
  module RethinkDB
    module BT_Mixin
      attr_accessor :highlight, :line

      def sanitize_context(context)
        if __FILE__ =~ /^(.*\/)[^\/]+.rb$/
          prefix = $1;
          context.reject{|x| x =~ /^#{prefix}/}
        else
          context
        end
      end

      def format_highlights(str)
        str.chars.map{|x| x == "\000" ? "^" : " "}.join.rstrip
      end

      def force_raise(query, debug=false)
        obj = query.run
        #maybe_reformat_err(query, obj);
        PP.pp obj if debug
        if obj.class == Query_Results
          obj = obj.to_a
        elsif obj.class == Hash
          raise RuntimeError,obj['first_error'] if obj['first_error']
        end
        obj
      end

      def maybe_reformat_err(query, obj)
        if obj.class == Hash && (err = obj['first_error'])
          obj['first_error'] = format_err(query, err)
        end
      end

      def format_err(query, err)
        parts = err.split("\nBacktrace:\n")
        errline = parts[0] || ""
        bt = (parts[1] && parts[1].split("\n")) || []
        errline+"\n"+query.print_backtrace(bt)
      end

      def set(sym,val)
        @map = {} if not @map
        res, @map[sym] = @map[sym], val
        return res
      end
      def consume sym
        @map = {} if not @map
        res, @map[sym] = @map[sym], nil
        return res.nil? ? 0 : res
      end
      def expand arg
        case arg
        when 'attr'           then []
        when 'attrname'       then []
        when 'base'           then [1+consume(:reduce)]
        when 'body'           then [4+consume(:reduce)]
        when 'datacenter'     then []
        when 'db_name'        then []
        when 'expr'           then [2]
        when 'false'          then [3]
        when 'group_mapping'  then [1, 1, 1]
        when 'key'            then [3]
        when 'keyname'        then []
        when 'lowerbound'     then [1, 2]
        when 'mapping'        then [1, 2]
        when 'modify_map'     then [2, 1]
        when 'order_by'       then []
        when 'point_map'      then [4, 1]
        when 'predicate'      then [1, 2]
        when 'reduce'         then [1]
        when 'reduction'      then set(:reduce, -1); [1, 3]
        when 'stream'         then [1]
        when 'table_name'     then []
        when 'table_ref'      then []
        when 'test'           then [1]
        when 'true'           then [2]
        when 'upperbound'     then [1, 3]
        when 'value_mapping'  then [1, 2, 1]
        when 'view'           then [1]
        when /arg:([0-9]+)/   then [2, $1.to_i]
        when /attrs:([0-9]+)/ then []
        when /elem:([0-9]+)/  then [$1.to_i+1]
        when /bind:(.+)$/     then [1, $1]
        when /key:(.+)$/      then [1..-1, $1]
        when /query:([0-9]+)/ then [3, $1.to_i]
        when /term:([0-9]+)/  then [2, $1.to_i]
        else  [:error]
        end
      end

      def recur(arr, lst, val)
        if lst[0].class == String
          entry = arr[0..-1].find{|x| x[0].to_s == lst[0]}
          raise RuntimeError if not entry
          mark(entry[1], lst[1..-1], val)
        elsif lst[0].class == Fixnum || lst[0].class == Range
          mark(arr[lst[0]], lst[1..-1], val) if lst != []
        else
          raise RuntimeError
        end
      end
      def mark(query, lst, val)
        #PP.pp [lst, query.class, query]
        if query.class == Array
          recur(query, lst, val) if lst != []
        elsif query.kind_of? RQL_Query
          if lst == []
            @line = sanitize_context(query.context)[0]
            val,query.marked = query.marked, val
            val
          else
            recur(query.body, lst, val)
          end
        else raise RuntimeError
        end
      end

      def with_marked_error(query, bt)
        @map = {}
        bt = bt.map{|x| expand x}.flatten
        raise RuntimeError if bt.any?{|x| x == :error}
        old = mark(query, bt, :error)
        res = yield
        mark(query, bt, old)
        return res
      end

      def with_highlight
        @highlight = true
        str = yield
        @highlight = false
        str.chars.map{|x| x == "\000" ? "^" : " "}.join.rstrip
      end

      def alt_inspect(query, &block)
        class << query
          attr_accessor :str_block
          def inspect; real_inspect({:str => @str_block.call(self)}); end
        end
        query.str_block = block
        query
      end
    end
    module BT; extend BT_Mixin; end
  end
  # Copyright 2010-2012 RethinkDB, all rights reserved.
  module RethinkDB
    module C_Mixin #Constants
      # We often have shorter names for things, or inline variants, specified here.
      def method_aliases
        { :attr => :getattr, :attrs => :pickattrs, :attr? => :hasattr,
          :get => :getattr, :pick => :pickattrs, :has => :hasattr,
          :equals => :eq, :neq => :ne,
          :sub => :subtract, :mul => :multiply, :div => :divide, :mod => :modulo,
          :+ => :add, :- => :subtract, :* => :multiply, :/ => :divide, :% => :modulo,
          :and => :all, :or => :any, :to_stream => :array_to_stream, :to_array => :stream_to_array} end

      # Allows us to identify protobuf classes which are actually variant types,
      # and to get their corresponding enums.
      def class_types
        { Query => Query::QueryType, WriteQuery => WriteQuery::WriteQueryType,
          Term => Term::TermType, Builtin => Builtin::BuiltinType,
          MetaQuery => MetaQuery::MetaQueryType} end

      # The protobuf spec often has a field with a slightly different name from
      # the enum constant, which we list here.
      def query_rewrites
        { :getattr => :attr,
          :hasattr => :attr,
          :without => :attrs,
          :pickattrs => :attrs,
          :string => :valuestring, :json => :jsonstring, :bool => :valuebool,
          :if => :if_, :getbykey => :get_by_key, :groupedmapreduce => :grouped_map_reduce,
          :insertstream => :insert_stream, :foreach => :for_each, :orderby => :order_by,
          :pointupdate => :point_update, :pointdelete => :point_delete,
          :pointmutate => :point_mutate, :concatmap => :concat_map,
          :read => :read_query, :write => :write_query, :meta => :meta_query,
          :create_db => :db_name, :drop_db => :db_name, :list_tables => :db_name
        } end

      def name_rewrites
        { :between => :range, :js => :javascript,
          :array_to_stream => :arraytostream, :stream_to_array => :streamtoarray,
          :merge => :mapmerge, :branch => :if, :pick => :pickattrs, :unpick => :without,
          :replace => :mutate, :pointreplace => :pointmutate, :contains => :hasattr,
          :count => :length
        } end

      # These classes go through a useless intermediate type.
      def trampolines; [:table, :map, :concatmap, :filter] end

      # These classes expect repeating arguments.
      def repeats; [:insert, :foreach]; end

      # These classes reference a tableref directly, rather than a term.
      def table_directs
        [:insert, :insertstream, :pointupdate, :pointdelete, :pointmutate,
         :create, :drop] end

      def nonatomic_variants
        [:update_nonatomic, :replace_nonatomic,
         :pointupdate_nonatomic, :pointreplace_nonatomic] end
    end
    module C; extend C_Mixin; end

    module P_Mixin #Protobuf Utils
      def enum_type(_class, _type); _class.values[_type.to_s.upcase.to_sym]; end
      def message_field(_class, _type); _class.fields.select{|x,y|
          y.instance_eval{@name} == _type
          #TODO: why did the following stop working?  (Or, why did it ever work?)
          #y.name == _type
        }[0]; end
      def message_set(message, key, val); message.send((key.to_s+'=').to_sym, val); end
    end
    module P; extend P_Mixin; end

    module S_Mixin #S-expression Utils
      def rewrite(q); C.name_rewrites[q] || q; end
      def mark_boolop(x); class << x; def boolop?; true; end; end; x; end
      def check_opts(opts, set)
        if (bad_opts = opts.map{|k,_|k} - set) != []
          raise ArgumentError,"Unrecognized options: #{bad_opts.inspect}"
        end
      end
      @@gensym_counter = 1000
      def gensym; '_var_'+(@@gensym_counter += 1).to_s; end
      def with_var
        sym = gensym
        yield sym, var(sym)
      end
      def var(varname)
        BT.alt_inspect(Var_Expression.new [:var, varname]) { |expr| expr.body[1] }
      end
      def r x; x.equal?(skip) ? x : RQL.expr(x); end
      def skip; :skip_2222ebd4_2c16_485e_8c27_bbe43674a852; end
      def conn_outdated; :conn_outdated_2222ebd4_2c16_485e_8c27_bbe43674a852; end

      def arg_or_block(*args, &block)
        if args.length == 1 && block
          raise ArgumentError,"Cannot provide both an argument *and* a block."
        end
        return lambda { args[0] } if (args.length == 1)
        return block if block
        raise ArgumentError,"Must provide either an argument or a block."
      end

      def clean_lst lst # :nodoc:
        return lst.sexp if lst.kind_of? RQL_Query
        return lst.class == Array ? lst.map{|z| clean_lst(z)} : lst
      end

      def replace(sexp, map)
        return sexp.map{|x| replace x,map} if sexp.class == Array
        return (map.include? sexp) ? map[sexp] : sexp
      end

      def checkdict s
        return s.to_s if s.class == String || s.class == Symbol
        raise RuntimeError, "Invalid JSON dict key: #{s.inspect}"
      end
    end
    module S; extend S_Mixin; end
  end
  # Copyright 2010-2012 RethinkDB, all rights reserved.
  module RethinkDB
    module RQL_Protob_Mixin
      include P_Mixin
      @@token = 0 # We need a new token every time we compile a query
      @@backtrace = []

      # Right now the only special case is :compare, but in general we might have
      # terms that don't follow the conventions and we need a place to store that.
      def handle_special_cases(message, query_type, query_args)
        #PP.pp ["special_case", message, query_type, query_args]
        case query_type
        when :compare then
          message.comparison = enum_type(Builtin::Comparison, *query_args)
          throw :unknown_comparator_error if not message.comparison
          return true
        else return false
        end
      end

      ################################################################################
      # I apologize for the code after this.  I was originally doing something
      # clever to generate the S-expressions, so they don't always match the
      # protobufs perfectly, and the complexity necessary to handle that got
      # pushed down into protobuf compilation.  Later the cleverness went away,
      # but I left the code below intact rather than refactoring for two reasons:
      #   1) It's fragile and took about a day to get right, so refactoring it
      #      probably isn't free.
      #   2) We are probably going to have to change protobuf implementations for
      #      speed reasons anyway, at which point this will have to be rewritten,
      #      and rewriting it twice doesn't make sense
      ################################################################################

      # Compile an RQL query into a protobuf.
      def comp(message_class, args, repeating=false)
        # H4x to make the backtraces print right until we fix the protobuf.
        message_class = S.rewrite(message_class);
        #PP.pp(["A", message_class, args, repeating]) #DEBUG

        # Handle special cases of arguments:
        #   * When we're compiling a repeating term, in which case we want to
        #     compile the elements of [args] rather than [args] itself.
        #   * Objects, which correspond to optional argument specifications.
        #   * Empty arguments
        return args.map {|arg| comp(message_class, arg)} if repeating
        args = args[0] if args.class == Array and args[0].class == Hash
        raise TypeError,"Can't build #{message_class} from #{args.inspect}." if args == []

        # Handle terminal parts of the protobuf, where we have to actually pack values.
        if message_class.kind_of? Symbol
          # It's easier for the user if we allow both atoms and 1-element lists
          args = [args] if args.class != Array
          if args.length != 1
          then raise TypeError,"Can't build #{message_class} from #{args.inspect}." end
          # Coercing symbols into strings makes our attribute notation more consistent
          args[0] = args[0].to_s if args[0].class == Symbol
          return args[0]
        end

        # Handle nonterminal parts of the protobuf, where we have to descend
        message = message_class.new
        if (message_type_class = C.class_types[message_class])
          query_type = S.rewrite(args[0]) # Our first argument is the type of our variant.
          message.type = enum_type(message_type_class, query_type)
          raise TypeError,"No type '#{query_type}' for '#{message_class}'."if !message.type

          if args.length == 1 # Our variant takes no arguments of its own.
            return message
          else # Compile our variant's arguments.
            query_args = args[1..-1]
            query_args = [query_args] if C.trampolines.include? query_type
            return message if handle_special_cases(message, query_type, query_args)

            # The general, non-special case.
            query_type = C.query_rewrites[query_type] || query_type
            field_metadata = message_class.fields.select{|x,y| y.name==query_type}.to_a[0]
            if not field_metadata
            then raise ArgumentError,"No field '#{query_type}' in '#{message_class}'." end
            field = field_metadata[1]
            message_set(message, query_type,
                        comp(field.type, query_args,field.rule==:repeated))
          end
          # Handle cases where we aren't constructing a toplevel variant type.
        elsif args.class == Array
          # Handle the case where we're constructinga the fields in order.
          args = args.map{|x| x == nil ? RQL.expr(x) : x}
          fields = message.fields.sort_by {|kv| kv[0]}
          fields.zip(args).each {|_params|;field = _params[0][1];arg = _params[1]
            if arg == S.skip
              if field.rule != :optional
                raise RuntimeError, "Cannot skip non-optional rule."
              end
            else
              message_set(message, field.name,
                          comp(field.type, arg, field.rule==:repeated)) if arg != nil
            end
          }
        else
          # Handle the case where the user provided neither an Array nor a Hash,
          # in which case they probably provided an Atom that we should treat as a
          # single argument (allowing this makes the interface cleaner).
          message = comp(message_class, [args], repeating)
        end
        return message
      rescue => e # Add our own, semantic backtrace to exceptions
        new_msg = repeating ? e.message :
          e.message + "\n...when compiling #{message_class} with:\n#{args.pretty_inspect}"
        raise e.class, new_msg
      end

      def handle_special_query_cases(sexp)
        if C.nonatomic_variants.include?(sexp[0])
          sexp[0] = sexp[0].to_s.split('_')[0].to_sym
          res = query(sexp)
          res.write_query.atomic = false
          return res
        end
        return nil
      end

      # Construct a protobuf query from an RQL query by inferring the query type.
      def query(sexp)
        raise TypeError, "Cannot build query from #{sexp.inspect}" if sexp.class != Array
        if (m = handle_special_query_cases(sexp))
        then q = m
        elsif enum_type(WriteQuery::WriteQueryType, S.rewrite(sexp[0]))
        then q = comp(Query, [:write, *sexp])
        elsif enum_type(MetaQuery::MetaQueryType, sexp[0])
        then q = comp(Query, [:meta, *sexp])
        else q = comp(Query, [:read, sexp])
        end
        q.token = (@@token += 1)
        return q
      end
    end
    module RQL_Protob; extend RQL_Protob_Mixin; end
  end
  # Copyright 2010-2012 RethinkDB, all rights reserved.

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
    # of 1000.  If you need to access values multiple times, use the <b>+to_a+</b>
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

    # A connection to the RethinkDB
    # cluster.  You need to create at least one connection before you can run
    # queries.  After creating a connection <b>+conn+</b> and constructing a
    # query <b>+q+</b>, the following are equivalent:
    #  conn.run(q)
    #  q.run
    # (This is because by default, invoking the <b>+run+</b> instance method on a
    # query runs it on the most-recently-opened connection.)
    #
    # ==== Attributes
    # * +default_db+ - The current default database (can be set with Connection#use).  Defaults to 'test'.
    # * +conn_id+ - You probably don't care about this.  Used in some advanced situations where you care about whether a connection object has reconnected.
    class Connection
      def inspect # :nodoc:
        properties = "(#{@host}:#{@port}) (Default Database: '#{@default_db}')"
        state = @listener ? "(listening)" : "(closed)"
        "#<RethinkDB::Connection:#{self.object_id} #{properties} #{state}>"
      end

      @@last = nil
      @@magic_number = 0xaf61ba35

      def debug_socket # :nodoc:
        @socket;
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
        @socket.write(packet)
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
      # basis in your query, specifying it again here won't override the
      # original choice.)
      #
      # <b>NOTE:</b> unlike most enumerable objects, you can only iterate over the
      # result once.  See RethinkDB::Query_Results for more details.
      def run (query, opts={:use_outdated => false})
        #File.open('sexp.txt', 'a') {|f| f.write(query.sexp.inspect+"\n")}
        is_atomic = (query.kind_of?(JSON_Expression) ||
                     query.kind_of?(Meta_Query) ||
                     query.kind_of?(Write_Query))
        map = {}
        map[:default_db] = @default_db if @default_db
        S.check_opts(opts, [:use_outdated])
        map[S.conn_outdated] = !!opts[:use_outdated]
        protob = query.query(map)
        if is_atomic
          a = []
          singular = token_iter(query, dispatch(protob)){|row| a.push row}
          a.each{|o| BT.maybe_reformat_err(query, o)}
          singular ? a : a[0]
        else
          return Query_Results.new(query, self, dispatch(protob))
        end
      end

      # Close the connection.
      def close
        @listener.terminate if @listener
        @listener = nil
        @socket.close
        @socket = nil
      end

      # Return the last opened connection, or throw if there is no such
      # connection.  Used by e.g. RQL_Query#run.
      def self.last
        return @@last if @@last
        raise RuntimeError, "No last connection.  Use RethinkDB::Connection.new."
      end

      def start_listener # :nodoc:
        class << @socket
          def read_exn(len) # :nodoc:
            buf = read len
            raise RuntimeError,"Connection closed by server." if !buf or buf.length != len
            return buf
          end
        end
        @socket.write([@@magic_number].pack('L<'))
        @listener.terminate if @listener
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
              Thread.current.terminate
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
    end
  end
  # Copyright 2010-2012 RethinkDB, all rights reserved.
  module RethinkDB
    # Data collectors are ways of aggregating data (for use with
    # Sequence#groupby).  You can define your own; all they are is a hash
    # containing a <b>+mapping+</b> to be applied to row, a
    # <b>+base+</b>/<b>+reduction+</b> to reduce over the mappings, and an
    # optional <b>+finalizer+</b> to call on the output of the reduction.  You can
    # expand the builtin data collectors below for examples.
    #
    # All of the builtin collectors can also be accessed using the <b>+r+</b>
    # shortcut (like <b>+r.sum+</b>).
    module Data_Collectors
      # Count the number of rows in each group.
      def self.count
        { :mapping => lambda{|row| 1},
          :base => 0,
          :reduction => lambda{|acc, val| acc + val} }
      end

      # Sum a particular attribute of the rows in each group.
      def self.sum(attr)
        { :mapping => lambda{|row| row[attr]},
          :base => 0,
          :reduction => lambda{|acc, val| acc + val} }
      end

      # Average a particular attribute of the rows in each group.
      def self.avg(attr)
        { :mapping => lambda{|row| [row[attr], 1]},
          :base => [0, 0],
          :reduction => lambda{|acc, val| [acc[0] + val[0], acc[1] + val[1]]},
          :finalizer => lambda{|res| res[0] / res[1]} }
      end
    end
  end
  # Copyright 2010-2012 RethinkDB, all rights reserved.
  #TODO: toplevel doc
  module RethinkDB
    class RQL_Query; end
    class Read_Query < RQL_Query # :nodoc:
    end
    class Write_Query < RQL_Query # :nodoc:
    end
    class Meta_Query < RQL_Query # :nodoc:
    end

    module Sequence; end
    class JSON_Expression < Read_Query; include Sequence; end
    class Single_Row_Selection < JSON_Expression; end
    class Stream_Expression < Read_Query; include Sequence; end
    class Multi_Row_Selection < Stream_Expression; end

    class Database; end
    class Table; end

    class Connection; end
    class Query_Results; include Enumerable; end

    module RQL; end

    # Shortcuts to easily build RQL queries.
    module Shortcuts
      # The only shortcut right now.  May be used as a wrapper to create
      # RQL values or as a shortcut to access function in the RQL
      # namespace.  For example, the following are equivalent:
      #   r.add(1, 2)
      #   RethinkDB::RQL.add(1, 2)
      #   r(1) + r(2)
      #   r(3)
      def r(*args)
        (args == [] ) ? RethinkDB::RQL : RQL.expr(*args)
      end
    end
  end
  # Copyright 2010-2012 RethinkDB, all rights reserved.
  module RethinkDB
    # An RQL query that can be sent to the RethinkDB cluster.  This class contains
    # only functions that work for any type of query.  Queries are either
    # constructed by methods in the RQL module, or by invoking the instance
    # methods of a query on other queries.
    class RQL_Query
      def boolop? # :nodoc:
        false
      end
      # Run the invoking query using the most recently opened connection.  See
      # Connection#run for more details.
      def run(*args)
        Connection.last.send(:run, self, *args)
      end

      def set_body(val, context=nil) # :nodoc:
        @context = context || caller
        @body = val
      end

      def initialize(init_body, context=nil) # :nodoc:
        set_body(init_body, context)
      end

      def ==(rhs) # :nodoc:
        raise ArgumentError,"
      Cannot use inline ==/!= with RQL queries, use .eq() instead if
      you want a query that does equality comparison.

      If you need to see whether two queries are the same, compare
      their S-expression representations like: `query1.sexp ==
      query2.sexp`."
      end

      # Return the S-expression representation of a query.  Used for
      # equality comparison and advanced debugging.
      def sexp
        S.clean_lst @body
      end
      attr_accessor :body, :marked, :context

      # Compile into either a protobuf class or a query.
      def as _class # :nodoc:
        RQL_Protob.comp(_class, sexp)
      end
      def query(map={S.conn_outdated => false}) # :nodoc:
        RQL_Protob.query(S.replace(sexp, map))
      end

      def print_backtrace(bt) # :nodoc:
        #PP.pp [bt, bt.map{|x| BT.expand x}.flatten, sexp]
        begin
          BT.with_marked_error(self, bt) {
            query = "Query: #{inspect}\n       #{BT.with_highlight {inspect}}"
            line = "Line: #{BT.line || "Unknown"}"
            line + "\n" + query
          }
        rescue Exception => e
          PP.pp [bt, e] if $DEBUG
          "<Internal error in query pretty printer.>"
        end
      end

      def pprint # :nodoc:
        return @body.inspect if @body.class != Array
        return "" if @body == []
        case @body[0]
        when :call then
          if @body[2].length == 1
            @body[2][0].inspect + "." + RQL_Query.new(@body[1],@context).inspect
          else
            func = @body[1][0].to_s
            func_args = @body[1][1..-1].map{|x| x.inspect}
            call_args = @body[2].map{|x| x.inspect}
            func + "(" + (func_args + call_args).join(", ") + ")"
          end
        else
          func = @body[0].to_s
          func_args = @body[1..-1].map{|x| x.inspect}
          func + "(" + func_args.join(", ") + ")"
        end
      end

      def real_inspect(args) # :nodoc:
        str = args[:str] || pprint
        (BT.highlight and @marked) ? "\000"*str.length : str
      end

      def inspect # :nodoc:
        real_inspect({})
      end

      # Dereference aliases (see utils.rb) and possibly dispatch to RQL
      # (because the caller may be trying to use the more convenient
      # inline version of an RQL function).
      def method_missing(m, *args, &block) # :nodoc:
        if (m2 = C.method_aliases[m]); then return self.send(m2, *args, &block); end
        if RQL.methods.map{|x| x.to_sym}.include?(m)
          return RQL.send(m, *[self, *args], &block)
        end
        super(m, *args, &block)
      end
    end
  end

  # Copyright 2010-2012 RethinkDB, all rights reserved.
  module RethinkDB
    # A database stored on the cluster.  Usually created with the <b>+r+</b>
    # shortcut, like:
    #   r.db('test')
    class Database
      # Refer to the database named <b>+name+</b>.  Usually you would
      # use the <b>+r+</b> shortcut instead:
      #   r.db(name)
      def initialize(name); @db_name = name.to_s; end

      # Access the table <b>+name+</b> in this database.  For example:
      #   r.db('test').table('tbl')
      # May also provide a set of options <b>+opts+</b>.  Right now the only
      # useful option is :use_outdated:
      #   r.db('test').table('tbl', {:use_outdated => true})
      def table(name, opts={:use_outdated => false}); Table.new(@db_name, name, opts); end

      # Create a new table in this database.  You may also optionally
      # specify the datacenter it should reside in, its primary key, and
      # its cache size.  For example:
      #   r.db('db').table_create('tbl', {:datacenter  => 'dc',
      #                                   :primary_key => 'id',
      #                                   :cache_size  => 1073741824})
      # When run, either returns <b>+nil+</b> or throws on error.
      def table_create(name, optargs={})
        S.check_opts(optargs, [:datacenter, :primary_key, :cache_size])
        dc = optargs[:datacenter] || S.skip
        pkey = optargs[:primary_key] || S.skip
        cache = optargs[:cache_size] || S.skip
        BT.alt_inspect(Meta_Query.new [:create_table, dc, [@db_name, name], pkey, cache]) {
          "db(#{@db_name.inspect}).create_table(#{name.inspect})"
        }
      end

      # Drop the table <b>+name+</b> from this database.  When run,
      # either returns <b>+nil+</b> or throws on error.
      def table_drop(name)
        BT.alt_inspect(Meta_Query.new [:drop_table, @db_name, name]) {
          "db(#{@db_name.inspect}).drop_table(#{name.inspect})"
        }
      end

      # List all the tables in this database.  When run, either returns
      # <b>+nil+</b> or throws on error.
      def table_list
        BT.alt_inspect(Meta_Query.new [:list_tables, @db_name]) {
          "db(#{@db_name.inspect}).list_tables"
        }
      end

      def inspect # :nodoc:
        real_inspect({:str => @db_name})
      end
    end

    # A table in a particular RethinkDB database.  If you call a
    # function from Sequence on it, it will be treated as a
    # Stream_Expression reading from the table.
    class Table
      def inspect # :nodoc:
        to_mrs.inspect
      end

      attr_accessor :opts
      # A table named <b>+name+</b> residing in database
      # <b>+db_name+</b>.  Usually you would instead write:
      #   r.db(db_name).table(name)
      def initialize(db_name, name, opts)
        @db_name = db_name;
        @table_name = name;
        @opts = opts
        @context = caller
        S.check_opts(@opts, [:use_outdated])
        use_od = (@opts.include? :use_outdated) ? @opts[:use_outdated] : S.conn_outdated
        @body = [:table, @db_name, @table_name, use_od]
      end

      # Insert one or more rows into the table.  For example:
      #   table.insert({:id => 0})
      #   table.insert([{:id => 1}, {:id => 2}])
      # If you try to insert a
      # row with a primary key already in the table, you will get back
      # an error.  For example, if you have a table <b>+table+</b>:
      #   table.insert([{:id => 1}, {:id => 1}])
      # Will return something like:
      #   {'inserted' => 1, 'errors' => 1, 'first_error' => ...}
      # If you want to overwrite a row, you should specifiy <b>+mode+</b> to be
      # <b>+:upsert+</b>, like so:
      #   table.insert([{:id => 1}, {:id => 1}], :upsert)
      # which will return:
      #   {'inserted' => 2, 'errors' => 0}
      # You may also provide a stream.  So to make a copy of a table, you can do:
      #   r.db_create('new_db').run
      #   r.db('new_db').table_create('new_table').run
      #   r.db('new_db').new_table.insert(table).run
      def insert(rows, mode=:new)
        raise_if_outdated
        rows = [rows] if rows.class != Array
        do_upsert = (mode == :upsert ? true : false)
        Write_Query.new [:insert, [@db_name, @table_name], rows.map{|x| S.r(x)}, do_upsert]
      end

      # Get the row  with key <b>+key+</b>.  You may also
      # optionally specify the name of the attribute to use as your key
      # (<b>+keyname+</b>), but note that your table must be indexed by that
      # attribute.  For example, if we have a table <b>+table+</b>:
      #   table.get(0)
      def get(key, keyname=:id)
        Single_Row_Selection.new [:getbykey, [@db_name, @table_name], keyname, S.r(key)]
      end

      def method_missing(m, *args, &block) # :nodoc:
        to_mrs.send(m, *args, &block);
      end

      def to_mrs # :nodoc:
        BT.alt_inspect(Multi_Row_Selection.new(@body, @context, @opts)) {
          "db(#{@db_name.inspect}).table(#{@table_name.inspect})"
        }
      end
    end
  end
  # Copyright 2010-2012 RethinkDB, all rights reserved.
  module RethinkDB
    # A "Sequence" is either a JSON array or a stream.  The functions in
    # this module may be invoked as instance methods of both JSON_Expression and
    # Stream_Expression, but you will get a runtime error if you invoke
    # them on a JSON_Expression that turns out not to be an array.
    module Sequence
      # For each element of the sequence, execute 1 or more write queries (to
      # execute more than 1, yield a list of write queries in the block).  For
      # example:
      #   table.for_each{|row| [table2.get(row[:id]).delete, table3.insert(row)]}
      # will, for each row in <b>+table+</b>, delete the row that shares its id
      # in <b>+table2+</b> and insert the row into <b>+table3+</b>.
      def for_each
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
      #   people.filter({:age => r.mul(2, 3)})
      def filter(obj=nil)
        if obj
          if obj.class == Hash         then self.filter { |row|
              JSON_Expression.new [:call, [:all], obj.map{|kv|
                                     row.getattr(kv[0]).eq(S.r(kv[1]))}]}
          else raise ArgumentError,"Filter: Not a hash: #{obj.inspect}."
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
      #   table.concat_map {|row| [row[:id], row[:id]*2]}
      #   table.map{|row| [row[:id], row[:id]*2]}.reduce([]){|a,b| r.union(a,b)}
      def concat_map
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
      #   table.filter{|row| (row[:id] >= 3) & (row[:id] <= 7)}
      # as are these:
      #   table.between(nil,7,:index)
      #   table.filter{|row| row[:index] <= 7}
      def between(start_key, end_key, keyname=:id)
        start_key = S.r(start_key || S.skip)
        end_key = S.r(end_key || S.skip)
        self.class.new [:call, [:between, keyname, start_key, end_key], [self]]
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
      # attributes from the object and returns only those.  If the input
      # is not an array, fails when the query is run.  The folling are
      # equivalent:
      #   r([{:a => 1, :b => 1, :c => 1},
      #      {:a => 2, :b => 2, :c => 2}]).pluck('a', 'b')
      #   r([{:a => 1, :b => 1}, {:a => 2, :b => 2}])
      def pluck(*args)
        self.map {|x| x.pick(*args)}
      end

      # For each element of a sequence, picks out the specified
      # attributes from the object and returns the residual object.  If
      # the input is not an array, fails when the query is run.  The
      # following are equivalent:
      #   r([{:a => 1, :b => 1, :c => 1},
      #      {:a => 2, :b => 2, :c => 2}]).without('a', 'b')
      #   r([{:c => 1}, {:c => 2}])
      def without(*args)
        self.map {|x| x.unpick(*args)}
      end

      # Order a sequence of objects by one or more attributes.  For
      # example, to sort first by name and then by social security
      # number for the table <b>+people+</b>, you could do:
      #   people.order_by(:name, :ssn)
      # By default order_by sorts in ascending order. To explicitly specify the
      # ordering wrap the attribute to be ordered by with r.asc or r.desc as in:
      #   people.order_by(r.desc(:name), :ssn)
      # which sorts first by name from Z-A and then by ssn from 0-9.
      def order_by(*orderings)
        orderings.map!{|x| x.class == Array ? x : [x, true]}
        self.class.new [:call, [:orderby, *orderings], [self]]
      end

      # Reduce a function over the sequence.  Note that unlike Ruby's reduce, you
      # cannot omit the base case.  The block you provide should take two
      # arguments, just like Ruby's reduce.  For example, if we have a table
      # <b>+table+</b>, the following will add up the <b>+count+</b> attribute of
      # all the rows:
      #   table.map{|row| row[:count]}.reduce(0){|a,b| a+b}
      # <b>NOTE:</b> unlike Ruby's reduce, this reduce only works on
      # sequences with elements of the same type as the base case.  For
      # example, the following is incorrect:
      #   table.reduce(0){|a,b| a + b[:count]} # INCORRECT
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
      #   table.grouped_map_reduce(lambda {|row| row[:id] % 4},
      #                            lambda {|row| row[:id]},
      #                            0,
      #                            lambda {|a,b| a+b})
      #   r([0,1,2,3]).map {|n|
      #     table.filter{|row| row[:id].eq(n)}.map{|row| row[:id]}.reduce(0){|a,b| a+b}
      #   }
      # Groupedmapreduce is more efficient than the second form because
      # it only has to traverse <b>+table+</b> once.
      def grouped_map_reduce(grouping, mapping, base, reduction)
        grouping_term = S.with_var{|vname,v| [vname, S.r(grouping.call(v))]}
        mapping_term = S.with_var{|vname,v| [vname, S.r(mapping.call(v))]}
        reduction_term = S.with_var {|aname, a| S.with_var {|bname, b|
            [S.r(base), aname, bname, S.r(reduction.call(a, b))]}}
        JSON_Expression.new [:call, [:groupedmapreduce,
                                     grouping_term,
                                     mapping_term,
                                     reduction_term],
                             [self]]
      end

      # Group a sequence by one or more attributes and return some data about each
      # group.  For example, if you have a table <b>+people+</b>:
      #   people.group_by(:name, :town, r.count).filter{|row| row[:reduction] > 1}
      # Will find all cases where two people in the same town share a name, and
      # return a list of those name/town pairs along with the number of people who
      # share that name in that town.  You can find a list of builtin data
      # collectors at Data_Collectors (which will also show you how to
      # define your own).
      def group_by(*args)
        raise ArgumentError,"group_by requires at least one argument" if args.length < 1
        attrs, opts = args[0..-2], args[-1]
        S.check_opts(opts, [:mapping, :base, :reduction, :finalizer])
        map = opts.has_key?(:mapping) ? opts[:mapping] : lambda {|row| row}
        if !opts.has_key?(:base) || !opts.has_key?(:reduction)
          raise TypeError, "Group by requires a reduction and base to be specified"
        end
        base = opts[:base]
        reduction = opts[:reduction]

        gmr = self.grouped_map_reduce(lambda{|r| attrs.map{|a| r[a]}}, map, base, reduction)
        if (f = opts[:finalizer])
          gmr = gmr.map{|group| group.merge({:reduction => f.call(group[:reduction])})}
        end
        return gmr
      end

      # Gets one or more elements from the sequence, much like [] in Ruby.
      # The following are all equivalent:
      #   r([1,2,3])
      #   r([0,1,2,3])[1...4]
      #   r([0,1,2,3])[1..3]
      #   r([0,1,2,3])[1..-1]
      # As are:
      #   r(1)
      #   r([0,1,2])[1]
      # And:
      #   r(2)
      #   r({:a => 2})[:a]
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
        else raise ArgumentError, "RQL_Query#[] can't handle #{ind.inspect}."
        end
      end

      # Return at most <b>+n+</b> elements from the sequence.  The
      # following are equivalent:
      #   r([1,2,3])
      #   r([1,2,3,4]).limit(3)
      #   r([1,2,3,4])[0...3]
      def limit(n); self[0...n]; end

      # Skip the first <b>+n+</b> elements of the sequence.  The following are equivalent:
      #   r([2,3,4])
      #   r([1,2,3,4]).skip(1)
      #   r([1,2,3,4])[1..-1]
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
      #   r([1,2,3])
      #   r([1,2,3,1]).distinct
      # And:
      #   r([1,2])
      #   r([{:x => 1}, {:x => 2}, {:x => 1}]).distinct(:x)
      def distinct(attr=nil);
        if attr then self.map{|row| row[attr]}.distinct
        else self.class.new [:call, [:distinct], [self]];
        end
      end

      # Get the length of the sequence.  If we have a table
      # <b>+table+</b> with at least 5 elements, the following are
      # equivalent:
      #   table[0...5].count
      #   r([1,2,3,4,5]).count
      def count(); JSON_Expression.new [:call, [:count], [self]]; end

      # Get element <b>+n+</b> of the sequence.  For example, the following are
      # equivalent:
      #   r(2)
      #   r([0,1,2,3]).nth(2)
      # (Note the 0-indexing.)
      def nth(n)
        JSON_Expression.new [:call, [:nth], [self, S.r(n)]]
      end

      # A normal inner join.  Takes as an argument the table to join with and a
      # block.  The block you provide should accept two tows and return
      # <b>+true+</b> if they should be joined or <b>+false+</b> otherwise.  For
      # example:
      #   table1.inner_join(table2) {|row1, row2| row1[:attr1] > row2[:attr2]}
      # Note that we don't merge the two tables when you do this.  The output will
      # be a list of objects like:
      #   {'left' => ..., 'right' => ...}
      # You can use Sequence#zip to get back a list of merged rows.
      def inner_join(other)
        self.concat_map {|row|
          other.concat_map {|row2|
            RQL.branch(yield(row, row2), [{:left => row, :right => row2}], [])
          }
        }
      end


      # A normal outer join.  Takes as an argument the table to join with and a
      # block.  The block you provide should accept two tows and return
      # <b>+true+</b> if they should be joined or <b>+false+</b> otherwise.  For
      # example:
      #   table1.outer_join(table2) {|row1, row2| row1[:attr1] > row2[:attr2]}
      # Note that we don't merge the two tables when you do this.  The output will
      # be a list of objects like:
      #   {'left' => ..., 'right' => ...}
      # You can use Sequence#zip to get back a list of merged rows.
      def outer_join(other)
        S.with_var {|vname, v|
          self.concat_map {|row|
            RQL.let({vname => other.concat_map {|row2|
                        RQL.branch(yield(row, row2),
                                   [{:left => row, :right => row2}],
                                   [])}.to_array}) {
              RQL.branch(v.count() > 0, v, [{:left => row}])
            }
          }
        }
      end

      # A special case of Sequence#inner_join that is guaranteed to run in
      # O(n*log(n)) time.  It does equality comparison between <b>+leftattr+</b> of
      # the invoking stream and the primary key of the <b>+other+</b> stream.  For
      # example, the following are equivalent (if <b>+id+</b> is the primary key
      # of <b>+table2+</b>):
      #   table1.eq_join(:a, table2)
      #   table2.inner_join(table2) {|row1, row2| r.eq row1[:a],row2[:id]}
      # As this function defaults to <b>+id+</b> for the primary key of the right
      # table, you will need to specify the primary key if it is otherwise:
      #   table1.eq_join(:a, table2, :t2pk)
      def eq_join(leftattr, other, rightattr=:id)
        S.with_var {|vname, v|
          self.concat_map {|row|
            RQL.let({vname => other.get(row[leftattr], rightattr)}) {
              RQL.branch(v.ne(nil), [{:left => row, :right => v}], [])
            }
          }
        }
      end

      # Take the output of Sequence#inner_join, Sequence#outer_join, or
      # Sequence#eq_join and merge the results together.  The following are
      # equivalent:
      #   table1.eq_join(:id, table2).zip
      #   table1.eq_join(:id, table2).map{|obj| obj['left'].merge(obj['right'])}
      def zip
        self.map {|row|
          RQL.branch(row.contains('right'), row['left'].merge(row['right']), row['left'])
        }
      end
    end
  end
  # Copyright 2010-2012 RethinkDB, all rights reserved.
  module RethinkDB
    # A lazy sequence of rows, e.g. what we get when reading from a table.
    # Includes the Sequence module, so look there for the methods.
    class Stream_Expression
      # Convert a stream into an array.  THINK CAREFULLY BEFORE DOING
      # THIS.  You can do more with an array than you can with a stream
      # (e.g., you can store an array in a variable), but that's because
      # arrays are stored in memory.  If your stream is big (e.g. you're
      # reading a giant table), that has serious performance
      # implications.  Also, if you return an array instead of a stream
      # from a query, the whole thing gets sent over the network at once
      # instead of lazily consuming chunks.  If you have a table <b>+table+</b> with at
      # least 3 elements, the following are equivalent:
      #   r[[1,1,1]]
      #   table.limit(3).map{1}.stream_to_array
      def stream_to_array(); JSON_Expression.new [:call, [:stream_to_array], [self]]; end
    end

    # A special case of Stream_Expression that you can write to.  You
    # will get a Multi_Row_Selection from most operations that access
    # tables.  For example, consider the following two queries:
    #   q1 = table.filter{|row| row[:id] < 5}
    #   q2 = table.map{|row| row[:id]}
    # The first query simply accesses some elements of a table, while the
    # second query does some work to produce a new stream.
    # Correspondingly, the first query returns a Multi_Row_Selection
    # while the second query returns a Stream_Expression.  So:
    #   q1.delete
    # is a legal query that will delete everything with <b>+id+</b> less
    # than 5 in <b>+table+</b>.  But:
    #   q2.delete
    # will raise an error.
    class Multi_Row_Selection < Stream_Expression
      attr_accessor :opts
      def initialize(body, context=nil, opts=nil) # :nodoc:
        super(body, context)
        if opts
          @opts = opts
        elsif @body[0] == :call and @body[2] and @body[2][0].kind_of? Multi_Row_Selection
          @opts = @body[2][0].opts
        end
      end

      def raise_if_outdated # :nodoc:
        if @opts and @opts[:use_outdated]
          raise RuntimeError, "Cannot write to outdated table."
        end
      end

      # Delete all of the selected rows.  For example, if we have
      # a table <b>+table+</b>:
      #   table.filter{|row| row[:id] < 5}.delete
      # will delete everything with <b>+id+</b> less than 5 in <b>+table+</b>.
      def delete
        raise_if_outdated
        Write_Query.new [:delete, self]
      end

      # Update all of the selected rows.  For example, if we have a table <b>+table+</b>:
      #   table.filter{|row| row[:id] < 5}.update{|row| {:score => row[:score]*2}}
      # will double the score of everything with <b>+id+</b> less than 5.
      # If the object returned in <b>+update+</b> has attributes
      # which are not present in the original row, those attributes will
      # still be added to the new row.
      #
      # If you want to do a non-atomic update, you should pass
      # <b>+:non_atomic+</b> as the optional variant:
      #   table.update(:non_atomic){|row| r.js("...")}
      # You need to do a non-atomic update when the block provided to update can't
      # be proved deterministic (e.g. if it contains javascript or reads from
      # another table).
      def update(variant=nil)
        raise_if_outdated
        S.with_var {|vname,v|
          Write_Query.new [:update, self, [vname, S.r(yield(v))]]
        }.apply_variant(variant)
      end

      # Replace all of the selected rows.  Unlike <b>+update+</b>, must return the
      # new row rather than an object containing attributes to be updated (may be
      # combined with RQL::merge to mimic <b>+update+</b>'s behavior).
      # May also return <b>+nil+</b> to delete the row.  For example, if we have a
      # table <b>+table+</b>, then:
      #   table.replace{|row| r.if(row[:id] < 5, nil, row)}
      # will delete everything with id less than 5, but leave the other rows untouched.
      #
      # If you want to do a non-atomic replace, you should pass
      # <b>+:non_atomic+</b> as the optional variant:
      #   table.replace(:non_atomic){|row| r.js("...")}
      # You need to do a non-atomic replace when the block provided to replace can't
      # be proved deterministic (e.g. if it contains javascript or reads from
      # another table).
      def replace(variant=nil)
        raise_if_outdated
        S.with_var {|vname,v|
          Write_Query.new [:replace, self, [vname, S.r(yield(v))]]
        }.apply_variant(variant)
      end
    end
  end
  # Copyright 2010-2012 RethinkDB, all rights reserved.
  module RethinkDB
    # A query returning a JSON expression.  Most of the functions that you
    # can run on a JSON object are found in RethinkDB::RQL and accessed
    # with the <b>+r+</b> shortcut.  For convenience, may of the
    # functions in Rethinkdb::RQL can be run as if they were instance
    # methods of JSON_Expression.  For example, the following are
    # equivalent:
    #   r.add(1, 2)
    #   r(1).add(2)
    #   r(3)
    #
    # Running a JSON_Expression query will return a literal Ruby
    # representation of the resulting JSON, rather than a stream or a
    # string.  For example, the following are equivalent:
    #   r.add(1,2).run
    #   3
    # As are:
    #   r({:a => 1, :b => 2}).pick(:a).run
    #   {'a' => 1}
    # (Note that the symbol keys were coerced into string keys in the
    # object.  JSON doesn't distinguish between symbols and strings.)
    class JSON_Expression
      # If <b>+ind+</b> is a symbol or a string, gets the corresponding
      # attribute of an object.  For example, the following are equivalent:
      #   r({:id => 1})[:id]
      #   r({:id => 1})['id']
      #   r(1)
      # Otherwise, if <b>+ind+</b> is a number or a range, invokes RethinkDB::Sequence#[]
      def [](ind)
        if ind.class == Symbol || ind.class == String
          BT.alt_inspect(JSON_Expression.new [:call, [:getattr, ind], [self]]) {
            "#{self.inspect}[#{ind.inspect}]"
          }
        else
          super
        end
      end

      # Append a single element to an array.  The following are equivalent:
      #   r([1,2,3,4])
      #   r([1,2,3]).append(4)
      def append(el)
        JSON_Expression.new [:call, [:arrayappend], [self, S.r(el)]]
      end

      # Get an attribute of a JSON object (the same as
      # JSON_Expression#[]).  The following are equivalent.
      #   r({:id => 1}).getattr(:id)
      #   r({:id => 1})[:id]
      #   r(1)
      def getattr(attrname)
        JSON_Expression.new [:call, [:getattr, attrname], [self]]
      end

      # Check whether a JSON object has all of the particular attributes.  The
      # following are equivalent:
      #   r({:id => 1, :val => 2}).contains(:id, :val)
      #   r(true)
      def contains(*attrnames)
        if attrnames.length == 1
          JSON_Expression.new [:call, [:contains, attrnames[0]], [self]]
        else
          self.contains(attrnames[0]) & self.contains(*attrnames[1..-1])
        end
      end

      # Construct a JSON object that has a subset of the attributes of
      # another JSON object by specifying which to keep.  The following are equivalent:
      #   r({:a => 1, :b => 2, :c => 3}).pick(:a, :c)
      #   r({:a => 1, :c => 3})
      def pick(*attrnames)
        JSON_Expression.new [:call, [:pick, *attrnames], [self]]
      end

      # Construct a JSON object that has a subset of the attributes of
      # another JSON object by specifying which to drop.  The following are equivalent:
      #   r({:a => 1, :b => 2, :c => 3}).without(:a, :c)
      #   r({:b => 2})
      def unpick(*attrnames)
        JSON_Expression.new [:call, [:unpick, *attrnames], [self]]
      end

      # Convert from an array to a stream.  While most sequence functions are polymorphic
      # and handle both arrays and streams, when arrays or streams need to be
      # combined (e.g. via <b>+union+</b>) you need to explicitly convert between
      # the types.  This is mostly for safety, but also because which type you're
      # working with affects error handling.  The following are equivalent:
      #   r([1,2,3]).arraytostream
      #   r([1,2,3]).to_stream
      def array_to_stream(); Stream_Expression.new [:call, [:array_to_stream], [self]]; end

      # Prefix numeric -.  The following are equivalent:
      #   -r(1)
      #   r(-1)
      def -@; JSON_Expression.new [:call, [:subtract], [self]]; end
      # Prefix numeric +.  The following are equivalent:
      #   +r(-1)
      #   r(-1)
      def +@; JSON_Expression.new [:call, [:add], [self]]; end
    end

    # A query representing a variable.  Produced by e.g. RQL::letvar.  This
    # is its own class because it needs to behave correctly when spliced
    # into a string (see RQL::js).
    class Var_Expression < JSON_Expression
      # Convert from an RQL query representing a variable to the name of that
      # variable.  Used e.g. in constructing javascript functions.
      def to_s
        if @body.class == Array and @body[0] == :var
        then @body[1]
        else raise TypeError, 'Can only call to_s on RQL_Queries representing variables.'
        end
      end
    end

    # Like Multi_Row_Selection, but for a single row.  E.g.:
    #   table.get(0)
    # yields a Single_Row_Selection
    class Single_Row_Selection < JSON_Expression
      # Analagous to Multi_Row_Selection#update
      def update(variant=nil)
        S.with_var {|vname,v|
          Write_Query.new [:pointupdate, *(@body[1..-1] + [[vname, S.r(yield(v))]])]
        }.apply_variant(variant)
      end

      # Analagous to Multi_Row_Selection#mutate
      def replace(variant=nil)
        S.with_var {|vname,v|
          Write_Query.new [:pointreplace, *(@body[1..-1] + [[vname, S.r(yield(v))]])]
        }.apply_variant(variant)
      end

      # Analagous to Multi_Row_Selection#delete
      def delete
        Write_Query.new [:pointdelete, *(@body[1..-1])]
      end
    end
  end
  # Copyright 2010-2012 RethinkDB, all rights reserved.
  module RethinkDB
    # A write operation, like an insert.
    class Write_Query < RQL_Query
      # Internal tool for when we have commands that take a flag but
      # which are most easily processed by the protobuf compiler as a
      # separate command.
      def apply_variant(variant) # :nodoc:
        return self if variant.nil?
        if variant == :non_atomic
          @body[0] = case @body[0]
                     when :update        then :update_nonatomic
                     when :replace       then :replace_nonatomic
                     when :pointupdate   then :pointupdate_nonatomic
                     when :pointreplace  then :pointreplace_nonatomic
                     else raise RuntimeError,"#{@body[0]} cannot be made nonatomic"
                     end
        else
          raise RuntimeError,"Unknown variant #{@body[0]}; did you mean `:non_atomic`?"
        end
        return self
      end
    end
  end

  # Copyright 2010-2012 RethinkDB, all rights reserved.
  module RethinkDB
    module Mixin_H4x # :nodoc:
      def &(l,r) # :nodoc:
        RQL.all_h4x(l,r)
      end
    end

    # This module contains the RQL query construction functions.  By far
    # the most common way of gaining access to those functions, however,
    # is to include/extend RethinkDB::Shortcuts to gain access to the
    # shortcut <b>+r+</b>.  That shortcut is used in all the examples
    # below.
    #
    # Also, many of the functions here can be called as if they were
    # instance methods of a RQL query.  So the following are all equivalent:
    #   r.add(1,2)
    #   r(1).add(2)
    #   r(1) + 2
    #   r.+(1, 2)
    module RQL
      # A shortcut for Connection::new
      def self.connect(*args, &block)
        Connection.new(*args, &block)
      end

      # Return the most recently opened connection.
      def self.last_connection; Connection.last; end

      # Construct a javascript expression, which may refer to variables in scope
      # (use <b>+to_s+</b> to get the name of a variable query, or simply splice
      # it in). Behaves as if passed to the standard `eval` function in
      # JavaScript. If you have a table <b>+table+</b>, the following are
      # equivalent:
      #   table.map{|row| row[:id]}
      #   table.map{|row| r.js("#{row}.id")}
      #   table.map{r.js("this.id")} #implicit variable
      #   table.map{r.js("var a = this.id; a;")} #implicit variable
      # As are:
      #   r.let(:a => 1, :b => 2) { r.add(r.letvar('a'), r.letvar('b'), 1) }
      #   r.let(:a => 1, :b => 2) { r.js('a+b+1') }
      def self.js(str);
        JSON_Expression.new [:js, str]
      end

      # Refer to the database named <b>+db_name+</b>.  Usually used as a
      # stepping stone to a table reference.  For instance, to refer to
      # the table 'tbl' in the database 'db':
      #   db('db').table('tbl')
      def self.db(db_name); Database.new db_name; end

      # Convert from a Ruby datatype to an RQL query.  Numbers, strings, booleans,
      # arrays, objects, and nil are all converted to their respective JSON types.
      # <b>Note:</b> this function is idempotent, so the following are equivalent:
      #   r.expr(5)
      #   r.expr(r.expr(5))
      # The shortcut `r` also acts like this when used as a function, so
      # the following are equivalent:
      #   r.expr(1)
      #   r(1)
      def self.expr x
        return x if x.kind_of? RQL_Query
        BT.alt_inspect(case x.class().hash
                       when Table.hash      then x.to_mrs
                       when String.hash     then JSON_Expression.new [:string, x]
                       when Fixnum.hash     then JSON_Expression.new [:number, x]
                       when Float.hash      then JSON_Expression.new [:number, x]
                       when TrueClass.hash  then JSON_Expression.new [:bool, x]
                       when FalseClass.hash then JSON_Expression.new [:bool, x]
                       when NilClass.hash   then JSON_Expression.new [:json_null]
                       when Array.hash      then JSON_Expression.new [:array, *x.map{|y| expr(y)}]
                       when Hash.hash       then
                         JSON_Expression.new [:object, *x.map{|var,term| [S.checkdict(var), expr(term)]}]

                       else raise TypeError, "RQL::expr can't handle object `#{x.inspect}` of class `#{x.class()}`.
Make sure you're providing a RQL expression or an object that can be coerced
to a JSON type (a String, Fixnum, Float, TrueClass, FalseClass, NilClass, Array,
or Hash)."
                       end) { x.inspect } end

      # Explicitly construct an RQL variable from a string.  See RQL::let.
      #   r.letvar('varname')
      def self.letvar(varname)
        BT.alt_inspect(Var_Expression.new [:var, varname]) { "letvar(#{varname.inspect})" }
      end

      # Provide a literal JSON string that will be parsed by the server.  For
      # example, the following are equivalent:
      #   r.expr({"a" => 5})
      #   r.json('{"a": 5}')
      def self.json(str); JSON_Expression.new [:json, str]; end

      # Construct an error.  This is usually used in the branch of an <b>+if+</b>
      # expression.  For example:
      #   r.if(r(1) > 2, false, r.error('unreachable'))
      # will only run the error query if 1 is greater than 2.  If an error query
      # does get run, it will be received as a RuntimeError in Ruby, so be
      # prepared to handle it.
      def self.error(err); JSON_Expression.new [:error, err]; end

      # Test a predicate and execute one of two branches (just like
      # Ruby's <b>+if+</b>).  For example, if we have a table
      # <b>+table+</b>:
      #   table.update{|row| r.branch(row[:score] < 10, {:score => 10}, {})}
      # will change every row with score below 10 in <b>+table+</b> to have score 10.
      def self.branch(test, t_branch, f_branch)
        tb = S.r(t_branch)
        fb = S.r(f_branch)
        if tb.kind_of? fb.class
          resclass = fb.class
        elsif fb.kind_of? tb.class
          resclass = tb.class
        else
          raise TypeError, "Both branches of IF must be of compatible types."
        end
        resclass.new [:branch, S.r(test), S.r(t_branch), S.r(f_branch)]
      end

      # Construct a query that binds some values to variable (as
      # specified by <b>+varbinds+</b>) and then executes <b>+body+</b>
      # with those variables accessible through RQL::letvar.  For
      # example, the following are equivalent:
      #   r.let(:a => 2, :b => 3) { r.letvar('a') + r.letvar('b') }
      #   r.expr(5)
      def self.let(varbinds, &body)
        varbinds = varbinds.to_a
        varbinds.map! {|name, value| [name.to_s, expr(value)]}
        res = S.r(body.call)
        res.class.new [:let, varbinds, res]
      end

      # Negate a boolean.  May also be called as if it were an instance method of
      # JSON_Expression for convenience.  The following are equivalent:
      #   r.not(true)
      #   r(true).not
      def self.not(pred); JSON_Expression.new [:call, [:not], [S.r(pred)]]; end

      # Add two or more numbers together.  May also be called as if it
      # were a instance method of JSON_Expression for convenience, and
      # overloads <b><tt>+</tt></b> if the lefthand side is a query.
      # The following are all equivalent:
      #   r.add(1,2)
      #   r(1).add(2)
      #   (r(1) + 2) # Note that (1 + r(2)) is *incorrect* because Ruby only
      #              # overloads based on the lefthand side.
      # The following is also legal:
      #   r.add(1,2,3)
      # Add may also be used to concatenate arrays.  The following are equivalent:
      #   r([1,2,3])
      #   r.add([1, 2], [3])
      #   r([1,2]) + [3]
      def self.add(a, b, *rest)
        JSON_Expression.new [:call, [:add], [S.r(a), S.r(b), *(rest.map{|x| S.r x})]];
      end

      # Subtract one number from another.
      # May also be called as if it were a instance method of JSON_Expression for
      # convenience, and overloads <b><tt>-</tt></b> if the lefthand side is a
      # query.  Also has the shorter synonym <b>+sub+</b>. The following are all
      # equivalent:
      #   r.subtract(1,2)
      #   r(1).subtract(2)
      #   r.sub(1,2)
      #   r(1).sub(2)
      #   (r(1) - 2) # Note that (1 - r(2)) is *incorrect* because Ruby only
      #              # overloads based on the lefthand side.
      def self.subtract(a, b); JSON_Expression.new [:call, [:subtract], [S.r(a), S.r(b)]]; end

      # Multiply two numbers together.  May also be called as if it were
      # a instance method of JSON_Expression for convenience, and
      # overloads <b><tt>+</tt></b> if the lefthand side is a query.
      # Also has the shorter synonym <b>+mul+</b>.  The following are
      # all equivalent:
      #   r.multiply(1,2)
      #   r(1).multiply(2)
      #   r.mul(1,2)
      #   r(1).mul(2)
      #   (r(1) * 2) # Note that (1 * r(2)) is *incorrect* because Ruby only
      #              # overloads based on the lefthand side.
      # The following is also legal:
      #   r.multiply(1,2,3)
      def self.multiply(a, b, *rest)
        JSON_Expression.new [:call, [:multiply], [S.r(a), S.r(b), *(rest.map{|x| S.r x})]];
      end

      # Divide one number by another.  May also be called as if it were
      # a instance method of JSON_Expression for convenience, and
      # overloads <b><tt>/</tt></b> if the lefthand side is a query.
      # Also has the shorter synonym <b>+div+</b>. The following are
      # all equivalent:
      #   r.divide(1,2)
      #   r(1).divide(2)
      #   r.div(1,2)
      #   r(1).div(2)
      #   (r(1) / 2) # Note that (1 / r(2)) is *incorrect* because Ruby only
      #              # overloads based on the lefthand side.
      def self.divide(a, b); JSON_Expression.new [:call, [:divide], [S.r(a), S.r(b)]]; end

      # Take one number modulo another.  May also be called as if it
      # were a instance method of JSON_Expression for convenience, and
      # overloads <b><tt>%</tt></b> if the lefthand side is a query.
      # Also has the shorter synonym <b>+mod+</b>. The following are all
      # equivalent:
      #   r.modulo(1,2)
      #   r(1).modulo(2)
      #   r.mod(1,2)
      #   r(1).mod(2)
      #   (r(1) % 2) # Note that (1 % r(2)) is *incorrect* because Ruby only
      #              # overloads based on the lefthand side.
      def self.modulo(a, b); JSON_Expression.new [:call, [:modulo], [S.r(a), S.r(b)]]; end

      # Returns true if any of its arguments are true, like <b>+or+</b>
      # in ruby (but with arbitrarily many arguments).  May also be
      # called as if it were a instance method of JSON_Expression for
      # convenience, and overloads <b><tt>|</tt></b> if the lefthand
      # side is a query.  Also has the synonym <b>+or+</b>.  The
      # following are all equivalent:
      #   r(true)
      #   r.any(false, true)
      #   r.or(false, true)
      #   r(false).any(true)
      #   r(false).or(true)
      #   (r(false) | true) # Note that (false | r(true)) is *incorrect* because
      #                     # Ruby only overloads based on the lefthand side
      def self.any(pred, *rest)
        JSON_Expression.new [:call, [:any], [S.r(pred), *(rest.map{|x| S.r x})]];
      end

      # Returns true if all of its arguments are true, like <b>+and+</b>
      # in ruby (but with arbitrarily many arguments).  May also be
      # called as if it were a instance method of JSON_Expression for
      # convenience, and overloads <b><tt>&</tt></b> if the lefthand
      # side is a query.  Also has the synonym <b>+and+</b>.  The
      # following are all equivalent:
      #   r(false)
      #   r.all(false, true)
      #   r.and(false, true)
      #   r(false).all(true)
      #   r(false).and(true)
      #   (r(false) & true) # Note that (false & r(true)) is *incorrect* because
      #                     # Ruby only overloads based on the lefthand side
      def self.all(pred, *rest)
        JSON_Expression.new [:call, [:all], [S.r(pred), *(rest.map{|x| S.r x})]];
      end

      # Take the union of 2 or more sequences <b>+seqs+</b>.  Note that
      # unlike normal set union, duplicate values are preserved.  May
      # also be called as if it were a instance method of RQL_Query,
      # for convenience.  For example, if we have a table
      # <b>+table+</b>, the following are equivalent:
      #   r.union(table.map{|row| r[:id]}, table.map{|row| row[:num]})
      #   table.map{|row| row[:id]}.union(table.map{|row| row[:num]})
      def self.union(*seqs)
        #TODO: this looks wrong...
        if seqs.all? {|x| x.kind_of? Stream_Expression}
          resclass = Stream_Expression
        elsif seqs.all? {|x| x.kind_of? JSON_Expression}
          resclass = JSON_Expression
        else
          seqs = seqs.map {|x| self.expr x}
          resclass = JSON_Expression
        end
        resclass.new [:call, [:union], seqs.map{|x| S.r x}];
      end

      # Merge two objects together with a preference for the object on the right.
      # The resulting object has all the attributes in both objects, and if the
      # two objects share an attribute, the value from the object on the right
      # "wins" and is included in the final object.  May also be called as if it
      # were an instance method of JSON_Expression, for convenience.  The following are
      # equivalent:
      #   r({:a => 10, :b => 2, :c => 30})
      #   r.merge({:a => 1, :b => 2}, {:a => 10, :c => 30})
      #   r({:a => 1, :b => 2}).merge({:a => 10, :c => 30})
      def self.merge(obj1, obj2)
        JSON_Expression.new [:call, [:merge], [S.r(obj1), S.r(obj2)]]
      end

      # Check whether two JSON expressions are equal.  May also be called as
      # if it were a member function of JSON_Expression for convenience.  Has the
      # synonym <b>+equals+</b>.  The following are all equivalent:
      #   r(true)
      #   r.eq 1,1
      #   r(1).eq(1)
      #   r.equals 1,1
      #   r(1).equals(1)
      # May also be used with more than two arguments.  The following are
      # equivalent:
      #   r(false)
      #   r.eq(1, 1, 2)
      def self.eq(*args)
        JSON_Expression.new [:call, [:compare, :eq], args.map{|x| S.r x}]
      end

      # Check whether two JSON expressions are *not* equal.  May also be
      # called as if it were a member function of JSON_Expression for convenience.  The
      # following are all equivalent:
      #   r(true)
      #   r.ne 1,2
      #   r(1).ne(2)
      #   r.not r.eq(1,2)
      #   r(1).eq(2).not
      # May also be used with more than two arguments.  The following are
      # equivalent:
      #   r(true)
      #   r.ne(1, 1, 2)
      def self.ne(*args)
        JSON_Expression.new [:call, [:compare, :ne], args.map{|x| S.r x}]
      end

      # Check whether one JSON expression is less than another.  May also be
      # called as if it were a member function of JSON_Expression for convenience.  May
      # also be called as the infix operator <b><tt> < </tt></b> if the lefthand
      # side is a query.  The following are all equivalent:
      #   r(true)
      #   r.lt 1,2
      #   r(1).lt(2)
      #   r(1) < 2
      # Note that the following is illegal, because Ruby only overloads infix
      # operators based on the lefthand side:
      #   1 < r(2)
      # May also be used with more than two arguments.  The following are
      # equivalent:
      #   r(true)
      #   r.lt(1, 2, 3)
      def self.lt(*args)
        JSON_Expression.new [:call, [:compare, :lt], args.map{|x| S.r x}]
      end

      # Check whether one JSON expression is less than or equal to another.
      # May also be called as if it were a member function of JSON_Expression for
      # convenience.  May also be called as the infix operator <b><tt> <= </tt></b>
      # if the lefthand side is a query.  The following are all equivalent:
      #   r(true)
      #   r.le 1,1
      #   r(1).le(1)
      #   r(1) <= 1
      # Note that the following is illegal, because Ruby only overloads infix
      # operators based on the lefthand side:
      #   1 <= r(1)
      # May also be used with more than two arguments.  The following are
      # equivalent:
      #   r(true)
      #   r.le(1, 2, 2)
      def self.le(*args)
        JSON_Expression.new [:call, [:compare, :le], args.map{|x| S.r x}]
      end

      # Check whether one JSON expression is greater than another.
      # May also be called as if it were a member function of JSON_Expression for
      # convenience.  May also be called as the infix operator <b><tt> > </tt></b>
      # if the lefthand side is an RQL query.  The following are all equivalent:
      #   r(true)
      #   r.gt 2,1
      #   r(2).gt(1)
      #   r(2) > 1
      # Note that the following is illegal, because Ruby only overloads infix
      # operators based on the lefthand side:
      #   2 > r(1)
      # May also be used with more than two arguments.  The following are
      # equivalent:
      #   r(true)
      #   r.gt(3, 2, 1)
      def self.gt(*args)
        JSON_Expression.new [:call, [:compare, :gt], args.map{|x| S.r x}]
      end

      # Check whether one JSON expression is greater than or equal to another.
      # May also be called as if it were a member function of JSON_Expression for
      # convenience.  May also be called as the infix operator <b><tt> >= </tt></b>
      # if the lefthand side is a query.  The following are all equivalent:
      #   r(true)
      #   r.ge 1,1
      #   r(1).ge(1)
      #   r(1) >= 1
      # Note that the following is illegal, because Ruby only overloads infix
      # operators based on the lefthand side:
      #   1 >= r(1)
      # May also be used with more than two arguments.  The following are
      # equivalent:
      #   r(true)
      #   r.ge(2, 2, 1)
      def self.ge(*args)
        JSON_Expression.new [:call, [:compare, :ge], args.map{|x| S.r x}]
      end

      # Create a new database with name <b>+db_name+</b>.  Either
      # returns <b>+nil+</b> or raises an error.
      def self.db_create(db_name); Meta_Query.new [:create_db, db_name]; end

      # List all databases.  Either returns an array of strings or raises an error.
      def self.db_list(); Meta_Query.new [:list_dbs]; end

      # Drop the database with name <b>+db_name+</b>.  Either returns
      # <b>+nil+</b> or raises an error.
      def self.db_drop(db_name); Meta_Query.new [:drop_db, db_name]; end

      # Dereference aliases (see utils.rb)
      def self.method_missing(m, *args, &block) # :nodoc:
        (m2 = C.method_aliases[m]) ? self.send(m2, *args, &block) : super(m, *args, &block)
      end

      # Refer to a table by name.  When run over a connection, this query uses the
      # default database of that connection.  If we have a connection <b>+c+</b>
      # like so:
      #   c = r.connect('localhost', 28015, 'db_name')
      # then the following are equivalent:
      #   c.run(r.table('tbl_name'))
      #   c.run(r.db('db_name').table('tbl_name')
      def self.table(name, opts={})
        Table.new(:default_db, name, opts)
      end

      # A shortcut for Data_Collectors::count
      def self.count(*args); Data_Collectors.count(*args); end
      # A shortcut for Data_Collectors::sum
      def self.sum(*args); Data_Collectors.sum(*args); end
      # A shortcut for Data_Collectors::avg
      def self.avg(*args); Data_Collectors.avg(*args); end

      # Specify ascending ordering for a given attribute passed to order_by.
      def self.asc(attr)
        return [attr, true]
      end

      # Specify descending ordering for a given attribute passed to order_by.
      def self.desc(attr)
        return [attr, false]
      end

      def self.boolprop(op, l, r) # :nodoc:
        badop = l.boolop? ? l : r
        if l.boolop? || r.boolop?
          raise RuntimeError,"Error: Cannot use infix #{op} operator on infix boolean expression:
#{badop.inspect}
This is almost always a precedence error; try adding parentheses.  If you
actually need to compare booleans, use non-infix operators like `r.all(a,b)`
instead of `a & b`."
        end
        return RQL.send(op, l, r)
      end
      # def self.boolprop(op, l, r) # :nodoc:
      #   if l.boolop?
      #     larg,rarg = l.body[2]
      #     sexp =  [l.body[0], l.body[1], [larg, boolprop(op, rarg, r)]]
      #   elsif r.boolop?
      #     larg,rarg = r.body[2]
      #     sexp =  [r.body[0], r.body[1], [boolprop(op, l, larg), rarg]]
      #   else
      #     return RQL.send(op, l, r);
      #   end
      #   return S.mark_boolop(JSON_Expression.new sexp)
      # end

      # See RQL::lt
      def self.< (l,r); boolprop(:lt, S.r(l), S.r(r)); end
      # See RQL::le
      def self.<=(l,r); boolprop(:le, S.r(l), S.r(r)); end
      # See RQL::gt
      def self.> (l,r); boolprop(:gt, S.r(l), S.r(r)); end
      # See RQL::ge
      def self.>=(l,r); boolprop(:ge, S.r(l), S.r(r)); end

      def self.|(l,r) # :nodoc:
        S.mark_boolop(any(l,r))
      end
      extend Mixin_H4x
      def self.all_h4x(l,r) # :nodoc:
        S.mark_boolop(all(l,r))
      end
    end
  end

  ### Generated by rprotoc. DO NOT EDIT!
  ### <proto file: ../../src/rdb_protocol/query_language.proto>
  # message TableRef {
  #     required string db_name    = 1;
  #     required string table_name = 2;
  #     optional bool use_outdated  = 3 [default = false]; //Use and outdated version of this table
  # };
  #
  # message VarTermTuple {
  #     required string var = 1;
  #     required Term term = 2;
  # };
  #
  # message Term {
  #     enum TermType {
  #         JSON_NULL = 0; //can't use just "NULL" here we get in trouble
  #         VAR = 1;
  #         LET = 2;
  #         CALL = 3;
  #         IF = 4;
  #         ERROR = 5;
  #         NUMBER = 6;
  #         STRING = 7;
  #         JSON = 8;
  #         BOOL = 9;
  #         ARRAY = 10;
  #         OBJECT = 11;
  #         GETBYKEY = 12;
  #         TABLE = 13;
  #         JAVASCRIPT = 14;
  #         IMPLICIT_VAR = 15;
  #     };
  #
  #     required TermType type = 1;
  #
  #     optional string var = 2;
  #
  #     message Let {
  #         repeated VarTermTuple binds = 1;
  #         required Term expr = 2;
  #     };
  #
  #     optional Let let = 3;
  #
  #     message Call {
  #         required Builtin builtin = 1;
  #         repeated Term args = 2;
  #     };
  #
  #     optional Call call = 4;
  #
  #     message If {
  #         required Term test = 1;
  #         required Term true_branch = 2;
  #         required Term false_branch = 3;
  #     };
  #
  #     optional If if_ = 5;
  #
  #     optional string error = 6;
  #
  #     optional double number = 7;
  #
  #     optional string valuestring = 8;
  #
  #     optional string jsonstring = 9;
  #
  #     optional bool valuebool = 10;
  #
  #     repeated Term array = 11;
  #
  #     repeated VarTermTuple object = 12;
  #     message GetByKey {
  #         required TableRef table_ref = 1;
  #         required string attrname = 2;
  #         required Term key = 3;
  #     };
  #
  #     optional GetByKey get_by_key = 14;
  #
  #     message Table {
  #         required TableRef table_ref = 1;
  #     };
  #
  #     optional Table table = 15;
  #
  #     optional string javascript = 16;
  #
  #     // reserved for RethinkDB internal use.
  #     extensions 1000 to 1099;
  # };
  #
  # message Builtin {
  #     enum BuiltinType {
  #         NOT = 1;
  #         GETATTR = 2;
  #         IMPLICIT_GETATTR = 3;
  #         HASATTR = 4;
  #         IMPLICIT_HASATTR = 5;
  #         PICKATTRS = 6;
  #         IMPLICIT_PICKATTRS = 7;
  #         MAPMERGE = 8;
  #         ARRAYAPPEND = 9;
  #         SLICE = 11;         // slice(thing_to_slice, None/left extent, None/right extent)
  #         ADD = 14;
  #         SUBTRACT = 15;
  #         MULTIPLY = 16;
  #         DIVIDE = 17;
  #         MODULO = 18;
  #         COMPARE = 19;
  #         FILTER = 20;
  #         MAP = 21;          // operates over streams
  #         CONCATMAP = 22;
  #         ORDERBY = 23;
  #         DISTINCT = 24;
  #         LENGTH = 26;
  #         UNION = 27;
  #         NTH = 28;
  #         STREAMTOARRAY = 29;
  #         ARRAYTOSTREAM = 30;
  #         REDUCE = 31;
  #         GROUPEDMAPREDUCE = 32;
  #         ANY = 35;
  #         ALL = 36;
  #         RANGE = 37;
  #         IMPLICIT_WITHOUT = 38;
  #         WITHOUT = 39;
  #     };
  #
  #     enum Comparison {
  #         EQ = 1;
  #         NE = 2;
  #         LT = 3;
  #         LE = 4;
  #         GT = 5;
  #         GE = 6;
  #     };
  #
  #     required BuiltinType type = 1;
  #
  #     optional string attr = 2; //used if type is GETATTR of HASATTR
  #
  #     repeated string attrs = 3; //used if type is PICKATTRS or WITHOUT
  #
  #     optional Comparison comparison = 4;
  #
  #     message Filter {
  #         required Predicate predicate = 1;
  #     };
  #
  #     optional Filter filter = 5;
  #
  #     message Map {
  #         required Mapping mapping = 1;
  #     };
  #
  #     optional Map map = 6;
  #
  #     message ConcatMap {
  #         required Mapping mapping = 1;
  #     };
  #
  #     optional ConcatMap concat_map = 7;
  #
  #     message OrderBy {
  #         required string attr = 1;
  #         optional bool ascending = 2 [default = true];
  #     };
  #
  #     repeated OrderBy order_by = 8;
  #
  #     optional Reduction reduce = 9;
  #
  #     message GroupedMapReduce {
  #         required Mapping group_mapping = 1;
  #         required Mapping value_mapping = 2;
  #         required Reduction reduction = 3;
  #     };
  #
  #     optional GroupedMapReduce grouped_map_reduce = 10;
  #
  #     message Range {
  #         required string attrname = 1;
  #         optional Term lowerbound = 2;
  #         optional Term upperbound = 3;
  #     };
  #
  #     optional Range range = 11;
  # };
  #
  # message Reduction {
  #     required Term base = 1;
  #     required string var1 = 2;
  #     required string var2 = 3;
  #     required Term body = 4;
  # };
  #
  # message Mapping {
  #     required string arg = 1;
  #     required Term body = 2;
  #     extensions 1000 to 1099;
  # };
  #
  # message Predicate {
  #     required string arg = 1;
  #     required Term body = 2;
  # };
  #
  # message ReadQuery {
  #     required Term term = 1;
  #     optional int64 max_chunk_size = 2; //may be 0 for unlimited
  #     optional int64 max_age = 3;
  #
  #     // reserved for RethinkDB internal use.
  #     extensions 1000 to 1099;
  # };
  #
  # message WriteQuery {
  #     enum WriteQueryType {
  #         UPDATE = 1;
  #         DELETE = 2;
  #         MUTATE = 3;
  #         INSERT = 4;
  #         FOREACH = 6;
  #         POINTUPDATE = 7;
  #         POINTDELETE = 8;
  #         POINTMUTATE = 9;
  #     };
  #
  #     required WriteQueryType type = 1;
  #     optional bool atomic = 11 [default = true];
  #
  #     message Update {
  #         required Term view = 1;
  #         required Mapping mapping = 2;
  #     };
  #
  #     optional Update update = 2;
  #
  #     message Delete {
  #         required Term view = 1;
  #     };
  #
  #     optional Delete delete = 3;
  #
  #     message Mutate {
  #         required Term view = 1;
  #         required Mapping mapping = 2;
  #     };
  #
  #     optional Mutate mutate = 4;
  #
  #     message Insert {
  #         required TableRef table_ref = 1;
  #         repeated Term terms = 2;
  #         optional bool overwrite = 3 [default = false];
  #     };
  #
  #     optional Insert insert = 5;
  #
  #     message ForEach {
  #         required Term stream = 1;
  #         required string var = 2;
  #         repeated WriteQuery queries = 3;
  #     };
  #
  #     optional ForEach for_each = 7;
  #
  #     message PointUpdate {
  #         required TableRef table_ref = 1;
  #         required string attrname = 2;
  #         required Term key = 3;
  #         required Mapping mapping = 4;
  #     };
  #
  #     optional PointUpdate point_update = 8;
  #
  #     message PointDelete {
  #         required TableRef table_ref = 1;
  #         required string attrname = 2;
  #         required Term key = 3;
  #     };
  #
  #     optional PointDelete point_delete = 9;
  #
  #     message PointMutate {
  #         required TableRef table_ref = 1;
  #         required string attrname = 2;
  #         required Term key = 3;
  #         required Mapping mapping = 4;
  #     };
  #
  #     optional PointMutate point_mutate = 10;
  # };
  #
  # message MetaQuery {
  #     enum MetaQueryType {
  #         CREATE_DB     = 1; //db_name
  #         DROP_DB       = 2; //db_name
  #         LIST_DBS      = 3;
  #
  #         CREATE_TABLE  = 4;
  #         DROP_TABLE    = 5;
  #         LIST_TABLES   = 6; //db_name
  #     };
  #     required MetaQueryType type = 1;
  #     optional string db_name = 2;
  #
  #     message CreateTable {
  #         optional string datacenter  = 1;
  #         required TableRef table_ref = 3;
  #         optional string primary_key = 4 [default = "id"];
  #         optional int64 cache_size = 5 [default = 1073741824]; //notice that's a gigabyte right there.
  #     }
  #     optional CreateTable create_table = 3;
  #
  #     optional TableRef drop_table = 4;
  # };
  #
  # message Query {
  #     enum QueryType {
  #         READ       = 1; //Begin reading from a stream (and possibly finish if it's short)
  #         WRITE      = 2; //Write to a table (currently does not create a stream)
  #         CONTINUE   = 3; //Continue reading from a stream
  #         STOP       = 4; //Stop reading from a stream and release its resources
  #         META       = 5; //Meta operations that create, drop, or list databases/tables
  #     }
  #
  #     required QueryType type = 1;
  #
  #     required int64 token = 2;
  #
  #     optional ReadQuery read_query = 3;
  #     optional WriteQuery write_query = 4;
  #     optional MetaQuery meta_query = 5;
  # }
  #
  # message Response {
  #     enum StatusCode {
  #         SUCCESS_EMPTY = 0;
  #         SUCCESS_JSON = 1;
  #         SUCCESS_PARTIAL = 2; //need to send CONTINUE repeatedly until SUCCESS_STREAM
  #         SUCCESS_STREAM = 3;
  #
  #         BROKEN_CLIENT = 101; // Means the client is misbehaving, not the user
  #         BAD_QUERY = 102;
  #         RUNTIME_ERROR = 103;
  #     }
  #     required StatusCode status_code = 1;
  #
  #     required int64 token = 2;
  #
  #     repeated string response = 3;
  #
  #     optional string error_message = 4;
  #
  #     message Backtrace {
  #         repeated string frame = 1;
  #     };
  #
  #     optional Backtrace backtrace = 5;
  # }

  require 'protobuf/message/message'
  require 'protobuf/message/enum'
  require 'protobuf/message/service'
  require 'protobuf/message/extend'

  class TableRef < ::Protobuf::Message
    defined_in __FILE__
    required :string, :db_name, 1
    required :string, :table_name, 2
    optional :bool, :use_outdated, 3, :default => false
  end
  class VarTermTuple < ::Protobuf::Message
    defined_in __FILE__
    required :string, :var, 1
    required :Term, :term, 2
  end
  class Term < ::Protobuf::Message
    defined_in __FILE__
    class TermType < ::Protobuf::Enum
      defined_in __FILE__
      JSON_NULL = value(:JSON_NULL, 0)
      VAR = value(:VAR, 1)
      LET = value(:LET, 2)
      CALL = value(:CALL, 3)
      IF = value(:IF, 4)
      ERROR = value(:ERROR, 5)
      NUMBER = value(:NUMBER, 6)
      STRING = value(:STRING, 7)
      JSON = value(:JSON, 8)
      BOOL = value(:BOOL, 9)
      ARRAY = value(:ARRAY, 10)
      OBJECT = value(:OBJECT, 11)
      GETBYKEY = value(:GETBYKEY, 12)
      TABLE = value(:TABLE, 13)
      JAVASCRIPT = value(:JAVASCRIPT, 14)
      IMPLICIT_VAR = value(:IMPLICIT_VAR, 15)
    end
    required :TermType, :type, 1
    optional :string, :var, 2
    class Let < ::Protobuf::Message
      defined_in __FILE__
      repeated :VarTermTuple, :binds, 1
      required :Term, :expr, 2
    end
    optional :Let, :let, 3
    class Call < ::Protobuf::Message
      defined_in __FILE__
      required :Builtin, :builtin, 1
      repeated :Term, :args, 2
    end
    optional :Call, :call, 4
    class If < ::Protobuf::Message
      defined_in __FILE__
      required :Term, :test, 1
      required :Term, :true_branch, 2
      required :Term, :false_branch, 3
    end
    optional :If, :if_, 5
    optional :string, :error, 6
    optional :double, :number, 7
    optional :string, :valuestring, 8
    optional :string, :jsonstring, 9
    optional :bool, :valuebool, 10
    repeated :Term, :array, 11
    repeated :VarTermTuple, :object, 12
    class GetByKey < ::Protobuf::Message
      defined_in __FILE__
      required :TableRef, :table_ref, 1
      required :string, :attrname, 2
      required :Term, :key, 3
    end
    optional :GetByKey, :get_by_key, 14
    class Table < ::Protobuf::Message
      defined_in __FILE__
      required :TableRef, :table_ref, 1
    end
    optional :Table, :table, 15
    optional :string, :javascript, 16
    extensions 1000..1099
  end
  class Builtin < ::Protobuf::Message
    defined_in __FILE__
    class BuiltinType < ::Protobuf::Enum
      defined_in __FILE__
      NOT = value(:NOT, 1)
      GETATTR = value(:GETATTR, 2)
      IMPLICIT_GETATTR = value(:IMPLICIT_GETATTR, 3)
      HASATTR = value(:HASATTR, 4)
      IMPLICIT_HASATTR = value(:IMPLICIT_HASATTR, 5)
      PICKATTRS = value(:PICKATTRS, 6)
      IMPLICIT_PICKATTRS = value(:IMPLICIT_PICKATTRS, 7)
      MAPMERGE = value(:MAPMERGE, 8)
      ARRAYAPPEND = value(:ARRAYAPPEND, 9)
      SLICE = value(:SLICE, 11)
      ADD = value(:ADD, 14)
      SUBTRACT = value(:SUBTRACT, 15)
      MULTIPLY = value(:MULTIPLY, 16)
      DIVIDE = value(:DIVIDE, 17)
      MODULO = value(:MODULO, 18)
      COMPARE = value(:COMPARE, 19)
      FILTER = value(:FILTER, 20)
      MAP = value(:MAP, 21)
      CONCATMAP = value(:CONCATMAP, 22)
      ORDERBY = value(:ORDERBY, 23)
      DISTINCT = value(:DISTINCT, 24)
      LENGTH = value(:LENGTH, 26)
      UNION = value(:UNION, 27)
      NTH = value(:NTH, 28)
      STREAMTOARRAY = value(:STREAMTOARRAY, 29)
      ARRAYTOSTREAM = value(:ARRAYTOSTREAM, 30)
      REDUCE = value(:REDUCE, 31)
      GROUPEDMAPREDUCE = value(:GROUPEDMAPREDUCE, 32)
      ANY = value(:ANY, 35)
      ALL = value(:ALL, 36)
      RANGE = value(:RANGE, 37)
      IMPLICIT_WITHOUT = value(:IMPLICIT_WITHOUT, 38)
      WITHOUT = value(:WITHOUT, 39)
    end
    class Comparison < ::Protobuf::Enum
      defined_in __FILE__
      EQ = value(:EQ, 1)
      NE = value(:NE, 2)
      LT = value(:LT, 3)
      LE = value(:LE, 4)
      GT = value(:GT, 5)
      GE = value(:GE, 6)
    end
    required :BuiltinType, :type, 1
    optional :string, :attr, 2
    repeated :string, :attrs, 3
    optional :Comparison, :comparison, 4
    class Filter < ::Protobuf::Message
      defined_in __FILE__
      required :Predicate, :predicate, 1
    end
    optional :Filter, :filter, 5
    class Map < ::Protobuf::Message
      defined_in __FILE__
      required :Mapping, :mapping, 1
    end
    optional :Map, :map, 6
    class ConcatMap < ::Protobuf::Message
      defined_in __FILE__
      required :Mapping, :mapping, 1
    end
    optional :ConcatMap, :concat_map, 7
    class OrderBy < ::Protobuf::Message
      defined_in __FILE__
      required :string, :attr, 1
      optional :bool, :ascending, 2, :default => true
    end
    repeated :OrderBy, :order_by, 8
    optional :Reduction, :reduce, 9
    class GroupedMapReduce < ::Protobuf::Message
      defined_in __FILE__
      required :Mapping, :group_mapping, 1
      required :Mapping, :value_mapping, 2
      required :Reduction, :reduction, 3
    end
    optional :GroupedMapReduce, :grouped_map_reduce, 10
    class Range < ::Protobuf::Message
      defined_in __FILE__
      required :string, :attrname, 1
      optional :Term, :lowerbound, 2
      optional :Term, :upperbound, 3
    end
    optional :Range, :range, 11
  end
  class Reduction < ::Protobuf::Message
    defined_in __FILE__
    required :Term, :base, 1
    required :string, :var1, 2
    required :string, :var2, 3
    required :Term, :body, 4
  end
  class Mapping < ::Protobuf::Message
    defined_in __FILE__
    required :string, :arg, 1
    required :Term, :body, 2
    extensions 1000..1099
  end
  class Predicate < ::Protobuf::Message
    defined_in __FILE__
    required :string, :arg, 1
    required :Term, :body, 2
  end
  class ReadQuery < ::Protobuf::Message
    defined_in __FILE__
    required :Term, :term, 1
    optional :int64, :max_chunk_size, 2
    optional :int64, :max_age, 3
    extensions 1000..1099
  end
  class WriteQuery < ::Protobuf::Message
    defined_in __FILE__
    class WriteQueryType < ::Protobuf::Enum
      defined_in __FILE__
      UPDATE = value(:UPDATE, 1)
      DELETE = value(:DELETE, 2)
      MUTATE = value(:MUTATE, 3)
      INSERT = value(:INSERT, 4)
      FOREACH = value(:FOREACH, 6)
      POINTUPDATE = value(:POINTUPDATE, 7)
      POINTDELETE = value(:POINTDELETE, 8)
      POINTMUTATE = value(:POINTMUTATE, 9)
    end
    required :WriteQueryType, :type, 1
    optional :bool, :atomic, 11, :default => true
    class Update < ::Protobuf::Message
      defined_in __FILE__
      required :Term, :view, 1
      required :Mapping, :mapping, 2
    end
    optional :Update, :update, 2
    class Delete < ::Protobuf::Message
      defined_in __FILE__
      required :Term, :view, 1
    end
    optional :Delete, :delete, 3
    class Mutate < ::Protobuf::Message
      defined_in __FILE__
      required :Term, :view, 1
      required :Mapping, :mapping, 2
    end
    optional :Mutate, :mutate, 4
    class Insert < ::Protobuf::Message
      defined_in __FILE__
      required :TableRef, :table_ref, 1
      repeated :Term, :terms, 2
      optional :bool, :overwrite, 3, :default => false
    end
    optional :Insert, :insert, 5
    class ForEach < ::Protobuf::Message
      defined_in __FILE__
      required :Term, :stream, 1
      required :string, :var, 2
      repeated :WriteQuery, :queries, 3
    end
    optional :ForEach, :for_each, 7
    class PointUpdate < ::Protobuf::Message
      defined_in __FILE__
      required :TableRef, :table_ref, 1
      required :string, :attrname, 2
      required :Term, :key, 3
      required :Mapping, :mapping, 4
    end
    optional :PointUpdate, :point_update, 8
    class PointDelete < ::Protobuf::Message
      defined_in __FILE__
      required :TableRef, :table_ref, 1
      required :string, :attrname, 2
      required :Term, :key, 3
    end
    optional :PointDelete, :point_delete, 9
    class PointMutate < ::Protobuf::Message
      defined_in __FILE__
      required :TableRef, :table_ref, 1
      required :string, :attrname, 2
      required :Term, :key, 3
      required :Mapping, :mapping, 4
    end
    optional :PointMutate, :point_mutate, 10
  end
  class MetaQuery < ::Protobuf::Message
    defined_in __FILE__
    class MetaQueryType < ::Protobuf::Enum
      defined_in __FILE__
      CREATE_DB = value(:CREATE_DB, 1)
      DROP_DB = value(:DROP_DB, 2)
      LIST_DBS = value(:LIST_DBS, 3)
      CREATE_TABLE = value(:CREATE_TABLE, 4)
      DROP_TABLE = value(:DROP_TABLE, 5)
      LIST_TABLES = value(:LIST_TABLES, 6)
    end
    required :MetaQueryType, :type, 1
    optional :string, :db_name, 2
    class CreateTable < ::Protobuf::Message
      defined_in __FILE__
      optional :string, :datacenter, 1
      required :TableRef, :table_ref, 3
      optional :string, :primary_key, 4, :default => "id"
      optional :int64, :cache_size, 5, :default => 1073741824
    end
    optional :CreateTable, :create_table, 3
    optional :TableRef, :drop_table, 4
  end
  class Query < ::Protobuf::Message
    defined_in __FILE__
    class QueryType < ::Protobuf::Enum
      defined_in __FILE__
      READ = value(:READ, 1)
      WRITE = value(:WRITE, 2)
      CONTINUE = value(:CONTINUE, 3)
      STOP = value(:STOP, 4)
      META = value(:META, 5)
    end
    required :QueryType, :type, 1
    required :int64, :token, 2
    optional :ReadQuery, :read_query, 3
    optional :WriteQuery, :write_query, 4
    optional :MetaQuery, :meta_query, 5
  end
  class Response < ::Protobuf::Message
    defined_in __FILE__
    class StatusCode < ::Protobuf::Enum
      defined_in __FILE__
      SUCCESS_EMPTY = value(:SUCCESS_EMPTY, 0)
      SUCCESS_JSON = value(:SUCCESS_JSON, 1)
      SUCCESS_PARTIAL = value(:SUCCESS_PARTIAL, 2)
      SUCCESS_STREAM = value(:SUCCESS_STREAM, 3)
      BROKEN_CLIENT = value(:BROKEN_CLIENT, 101)
      BAD_QUERY = value(:BAD_QUERY, 102)
      RUNTIME_ERROR = value(:RUNTIME_ERROR, 103)
    end
    required :StatusCode, :status_code, 1
    required :int64, :token, 2
    repeated :string, :response, 3
    optional :string, :error_message, 4
    class Backtrace < ::Protobuf::Message
      defined_in __FILE__
      repeated :string, :frame, 1
    end
    optional :Backtrace, :backtrace, 5
  end

  class Runner
    def initialize(host, port)
      extend RethinkDB::Shortcuts
      @c = RethinkDB::Connection.new(host, port, 'test')
    end
    def db_list
      r.db_list.run
    end
    def table_list(db)
      r.db(db).table_list.run
    end
    def rows(db, tbl)
      r.db(db).table(tbl).run
    end
    def db_create(db)
      r.db_create(db).run if db != 'test'
    end
    def table_create(db, tbl, key)
      r.db(db).table_create(tbl, {:primary_key => key}).run
    end
    def insert_rows(db, tbl, rows)
      r.db(db).table(tbl).insert(rows).run
    end
  end
end

module Query_Language_2
  # Copyright 2010-2012 RethinkDB, all rights reserved.

  ### Generated by rprotoc. DO NOT EDIT!
  ### <proto file: ../../src/rdb_protocol/ql2.proto>
  # ////////////////////////////////////////////////////////////////////////////////
  # //                            THE HIGH-LEVEL VIEW                             //
  # ////////////////////////////////////////////////////////////////////////////////
  #
  # // Process: First send the magic number for the version of the protobuf you're
  # // targetting (in the [Version] enum).  This should **NOT** be sent as a
  # // protobuf; just send the little-endian 32-bit integer over the wire raw.
  # // Next, send [Query] protobufs and wait for [Response] protobufs with the same
  # // token.  You can see an example exchange below in **EXAMPLE**.
  #
  # // A query consists of a [Term] to evaluate and a unique-per-connection
  # // [token].
  #
  # // Tokens are used for two things:
  # // * Keeping track of which responses correspond to which queries.
  # // * Batched queries.  Some queries return lots of results, so we send back
  # //   batches of <1000, and you need to send a [CONTINUE] query with the same
  # //   token to get more results from the original query.
  # ////////////////////////////////////////////////////////////////////////////////
  #
  # // This enum contains the magic numbers for your version.  See **THE HIGH-LEVEL
  # // VIEW** for what to do with it.
  # message VersionDummy { // We need to wrap it like this for some
  #                        // non-conforming protobuf libraries
  #     enum Version {
  #         V0_1 = 0x3f61ba36;
  #     };
  # };
  #
  # // You send one of:
  # // * A [START] query with a [Term] to evaluate and a unique-per-connection token.
  # // * A [CONTINUE] query with the same token as a [START] query that returned
  # //   [SUCCESS_PARTIAL] in its [Response].
  # // * A [STOP] query with the same token as a [START] query that you want to stop.
  # message Query {
  #     enum QueryType {
  #         START    = 1; // Start a new query.
  #         CONTINUE = 2; // Continue a query that returned [SUCCESS_PARTIAL]
  #                       // (see [Response]).
  #         STOP     = 3; // Stop a query partway through executing.
  #     };
  #     optional QueryType type = 1;
  #     // A [Term] is how we represent the operations we want a query to perform.
  #     optional Term query = 2; // only present when [type] = [START]
  #     optional int64 token = 3;
  #     optional bool noreply = 4 [default = false]; // CURRENTLY IGNORED, NO SERVER SUPPORT
  #
  #     message AssocPair {
  #         optional string key = 1;
  #         optional Term val = 2;
  #     };
  #     repeated AssocPair global_optargs = 6;
  # };
  #
  # // A backtrace frame (see `backtrace` in Response below)
  # message Frame {
  #     enum FrameType {
  #         POS = 1; // Error occured in a positional argument.
  #         OPT = 2; // Error occured in an optional argument.
  #     };
  #     optional FrameType type = 1;
  #     optional int64 pos = 2; // The index of the positional argument.
  #     optional string opt = 3; // The name of the optional argument.
  # };
  # message Backtrace {
  #     repeated Frame frames = 1;
  # }
  #
  # // You get back a response with the same [token] as your query.
  # message Response {
  #     enum ResponseType {
  #         // These response types indicate success.
  #         SUCCESS_ATOM     = 1; // Query returned a single RQL datatype.
  #         SUCCESS_SEQUENCE = 2; // Query returned a sequence of RQL datatypes.
  #         SUCCESS_PARTIAL  = 3; // Query returned a partial sequence of RQL
  #                               // datatypes.  If you send a [CONTINUE] query with
  #                               // the same token as this response, you will get
  #                               // more of the sequence.  Keep sending [CONTINUE]
  #                               // queries until you get back [SUCCESS_SEQUENCE].
  #
  #         // These response types indicate failure.
  #         CLIENT_ERROR  = 16; // Means the client is buggy.  An example is if the
  #                             // client sends a malformed protobuf, or tries to
  #                             // send [CONTINUE] for an unknown token.
  #         COMPILE_ERROR = 17; // Means the query failed during parsing or type
  #                             // checking.  For example, if you pass too many
  #                             // arguments to a function.
  #         RUNTIME_ERROR = 18; // Means the query failed at runtime.  An example is
  #                             // if you add together two values from a table, but
  #                             // they turn out at runtime to be booleans rather
  #                             // than numbers.
  #     };
  #     optional ResponseType type = 1;
  #     optional int64 token = 2; // Indicates what [Query] this response corresponds to.
  #
  #     // [response] contains 1 RQL datum if [type] is [SUCCESS_ATOM], or many RQL
  #     // data if [type] is [SUCCESS_SEQUENCE] or [SUCCESS_PARTIAL].  It contains 1
  #     // error message (of type [R_STR]) in all other cases.
  #     repeated Datum response = 3;
  #
  #     // If [type] is [CLIENT_ERROR], [TYPE_ERROR], or [RUNTIME_ERROR], then a
  #     // backtrace will be provided.  The backtrace says where in the query the
  #     // error occured.  Ideally this information will be presented to the user as
  #     // a pretty-printed version of their query with the erroneous section
  #     // underlined.  A backtrace is a series of 0 or more [Frame]s, each of which
  #     // specifies either the index of a positional argument or the name of an
  #     // optional argument.  (Those words will make more sense if you look at the
  #     // [Term] message below.)
  #
  #     optional Backtrace backtrace = 4; // Contains n [Frame]s when you get back an error.
  # };
  #
  # // A [Datum] is a chunk of data that can be serialized to disk or returned to
  # // the user in a Response.  Currently we only support JSON types, but we may
  # // support other types in the future (e.g., a date type or an integer type).
  # message Datum {
  #     enum DatumType {
  #         R_NULL   = 1;
  #         R_BOOL   = 2;
  #         R_NUM    = 3; // a double
  #         R_STR    = 4;
  #         R_ARRAY  = 5;
  #         R_OBJECT = 6;
  #     };
  #     optional DatumType type = 1;
  #     optional bool r_bool = 2;
  #     optional double r_num = 3;
  #     optional string r_str = 4;
  #
  #     repeated Datum r_array = 5;
  #     message AssocPair {
  #         optional string key = 1;
  #         optional Datum val = 2;
  #     };
  #     repeated AssocPair r_object = 6;
  #
  #     extensions 10000 to 20000;
  # };
  #
  # // A [Term] is either a piece of data (see **Datum** above), or an operator and
  # // its operands.  If you have a [Datum], it's stored in the member [datum].  If
  # // you have an operator, its positional arguments are stored in [args] and its
  # // optional arguments are stored in [optargs].
  # //
  # // A note about type signatures:
  # // We use the following notation to denote types:
  # //   arg1_type, arg2_type, argrest_type... -> result_type
  # // So, for example, if we have a function `avg` that takes any number of
  # // arguments and averages them, we might write:
  # //   NUMBER... -> NUMBER
  # // Or if we had a function that took one number modulo another:
  # //   NUMBER, NUMBER -> NUMBER
  # // Or a function that takes a table and a primary key of any Datum type, then
  # // retrieves the entry with that primary key:
  # //   Table, DATUM -> OBJECT
  # // Some arguments must be provided as literal values (and not the results of sub
  # // terms).  These are marked with a `!`.
  # // Optional arguments are specified within curly braces as argname `:` value
  # // type (e.x `{use_outdated:BOOL}`)
  # // The RQL type hierarchy is as follows:
  # //   Top
  # //     DATUM
  # //       NULL
  # //       BOOL
  # //       NUMBER
  # //       STRING
  # //       OBJECT
  # //         SingleSelection
  # //       ARRAY
  # //     Sequence
  # //       ARRAY
  # //       Stream
  # //         StreamSelection
  # //           Table
  # //     Database
  # //     Function
  # //   Error
  # message Term {
  #     enum TermType {
  #         // A RQL datum, stored in `datum` below.
  #         DATUM = 1;
  #
  #         MAKE_ARRAY = 2; // DATUM... -> ARRAY
  #         // Evaluate the terms in [optargs] and make an object
  #         MAKE_OBJ   = 3; // {...} -> OBJECT
  #
  #         // * Compound types
  #         // Takes an integer representing a variable and returns the value stored
  #         // in that variable.  It's the responsibility of the client to translate
  #         // from their local representation of a variable to a unique integer for
  #         // that variable.  (We do it this way instead of letting clients provide
  #         // variable names as strings to discourage variable-capturing client
  #         // libraries, and because it's more efficient on the wire.)
  #         VAR          = 10; // !NUMBER -> DATUM
  #         // Takes some javascript code and executes it.
  #         JAVASCRIPT   = 11; // STRING -> DATUM | STRING -> Function(*)
  #         // Takes a string and throws an error with that message.
  #         ERROR        = 12; // STRING -> Error
  #         // Takes nothing and returns a reference to the implicit variable.
  #         IMPLICIT_VAR = 13; // -> DATUM
  #
  #         // * Data Operators
  #         // Returns a reference to a database.
  #         DB    = 14; // STRING -> Database
  #         // Returns a reference to a table.
  #         TABLE = 15; // Database, STRING, {use_outdated:BOOL} -> Table | STRING, {use_outdated:BOOL} -> Table
  #         // Gets a single element from a table by its primary key.
  #         GET   = 16; // Table, STRING -> SingleSelection | Table, NUMBER -> SingleSelection |
  #                     // Table, STRING -> NULL            | Table, NUMBER -> NULL
  #
  #         // Simple DATUM Ops
  #         EQ  = 17; // DATUM... -> BOOL
  #         NE  = 18; // DATUM... -> BOOL
  #         LT  = 19; // DATUM... -> BOOL
  #         LE  = 20; // DATUM... -> BOOL
  #         GT  = 21; // DATUM... -> BOOL
  #         GE  = 22; // DATUM... -> BOOL
  #         NOT = 23; // BOOL -> BOOL
  #         // ADD can either add two numbers or concatenate two arrays.
  #         ADD = 24; // NUMBER... -> NUMBER | STRING... -> STRING
  #         SUB = 25; // NUMBER... -> NUMBER
  #         MUL = 26; // NUMBER... -> NUMBER
  #         DIV = 27; // NUMBER... -> NUMBER
  #         MOD = 28; // NUMBER, NUMBER -> NUMBER
  #
  #         // DATUM Array Ops
  #         // Append a single element to the end of an array (like `snoc`).
  #         APPEND = 29; // ARRAY, DATUM -> ARRAY
  #         SLICE  = 30; // Sequence, NUMBER, NUMBER -> Sequence
  #         SKIP  = 70; // Sequence, NUMBER -> Sequence
  #         LIMIT = 71; // Sequence, NUMBER -> Sequence
  #
  #         // Stream/Object Ops
  #         // Get a particular attribute out of an object, or map that over a
  #         // sequence.
  #         GETATTR  = 31; // OBJECT, STRING -> DATUM
  #         // Check whether an object contains all of a set of attributes, or map
  #         // that over a sequence.
  #         CONTAINS = 32; // OBJECT, STRING... -> BOOL
  #         // Get a subset of an object by selecting some attributes to preserve,
  #         // or map that over a sequence.  (Both pick and pluck, polymorphic.)
  #         PLUCK    = 33; // Sequence, STRING... -> Sequence | OBJECT, STRING... -> OBJECT
  #         // Get a subset of an object by selecting some attributes to discard, or
  #         // map that over a sequence.  (Both unpick and without, polymorphic.)
  #         WITHOUT  = 34; // Sequence, STRING... -> Sequence | OBJECT, STRING... -> OBJECT
  #         // Merge objects (right-preferential)
  #         MERGE    = 35; // OBJECT... -> OBJECT | Sequence -> Sequence
  #
  #         // Sequence Ops
  #         // Get all elements of a sequence between two values.
  #         BETWEEN   = 36; // StreamSelection, {left_bound:DATUM, right_bound:DATUM} -> StreamSelection
  #         REDUCE    = 37; // Sequence, Function(2), {base:DATUM} -> DATUM
  #         MAP       = 38; // Sequence, Function(1) -> Sequence
  #         FILTER    = 39; // Sequence, Function(1) -> Sequence | Sequence, OBJECT -> Sequence
  #         // Map a function over a sequence and then concatenate the results together.
  #         CONCATMAP = 40; // Sequence, Function(1) -> Sequence
  #         // Order a sequence based on one or more attributes.
  #         ORDERBY   = 41; // Sequence, !STRING... -> Sequence
  #         // Get all distinct elements of a sequence (like `uniq`).
  #         DISTINCT  = 42; // Sequence -> Sequence
  #         // Count the number of elements in a sequence.
  #         COUNT     = 43; // Sequence -> NUMBER
  #         // Take the union of multiple sequences (preserves duplicate elements! (use distinct)).
  #         UNION     = 44; // Sequence... -> Sequence
  #         // Get the Nth element of a sequence.
  #         NTH       = 45; // Sequence, NUMBER -> DATUM
  #         // Takes a sequence, and three functions:
  #         // - A function to group the sequence by.
  #         // - A function to map over the groups.
  #         // - A reduction to apply to each of the groups.
  #         GROUPED_MAP_REDUCE = 46; // Sequence, Function(1), Function(1), Function(2), {base:DATUM} -> Sequence
  #         // Groups a sequence by one or more attributes, and then applies a reduction.
  #         GROUPBY            = 47; // Sequence, ARRAY, !OBJECT -> Sequence
  #         INNER_JOIN         = 48; // Sequence, Sequence, Function(2) -> Sequence
  #         OUTER_JOIN         = 49; // Sequence, Sequence, Function(2) -> Sequence
  #         // An inner-join that does an equality comparison on two attributes.
  #         EQ_JOIN            = 50; // Sequence, !STRING, Sequence -> Sequence
  #         ZIP                = 72; // Sequence -> Sequence
  #
  #
  #         // * Type Ops
  #         // Coerces a datum to a named type (e.g. "bool").
  #         // If you previously used `stream_to_array`, you should use this instead
  #         // with the type "array".
  #         COERCE_TO = 51; // Top, STRING -> Top
  #         // Returns the named type of a datum (e.g. TYPEOF(true) = "BOOL")
  #         TYPEOF = 52; // Top -> STRING
  #
  #         // * Write Ops (the OBJECTs contain data about number of errors etc.)
  #         // Updates all the rows in a selection.  Calls its Function with the row
  #         // to be updated, and then merges the result of that call.
  #         UPDATE   = 53; // StreamSelection, Function(1), {non_atomic:BOOL} -> OBJECT |
  #                        // SingleSelection, Function(1), {non_atomic:BOOL} -> OBJECT |
  #                        // StreamSelection, OBJECT,      {non_atomic:BOOL} -> OBJECT |
  #                        // SingleSelection, OBJECT,      {non_atomic:BOOL} -> OBJECT
  #         // Deletes all the rows in a selection.
  #         DELETE   = 54; // StreamSelection -> OBJECT | SingleSelection -> OBJECT
  #         // Replaces all the rows in a selection.  Calls its Function with the row
  #         // to be replaced, and then discards it and stores the result of that
  #         // call.
  #         REPLACE  = 55; // StreamSelection, Function(1), {non_atomic:BOOL} -> OBJECT | SingleSelection, Function(1), {non_atomic:BOOL} -> OBJECT
  #         // Inserts into a table.  If `upsert` is true, overwrites entries with
  #         // the same primary key (otherwise errors).
  #         INSERT   = 56; // Table, OBJECT, {upsert:BOOL} -> OBJECT | Table, Sequence, {upsert:BOOL} -> OBJECT
  #
  #         // * Administrative OPs
  #         // Creates a database with a particular name.
  #         DB_CREATE    = 57; // STRING -> OBJECT
  #         // Drops a database with a particular name.
  #         DB_DROP      = 58; // STRING -> OBJECT
  #         // Lists all the databases by name.  (Takes no arguments)
  #         DB_LIST      = 59; // -> ARRAY
  #         // Creates a table with a particular name in a particular database.
  #         TABLE_CREATE = 60; // Database, STRING, {datacenter:STRING, primary_key:STRING, cache_size:NUMBER} -> OBJECT
  #         // Drops a table with a particular name from a particular database.
  #         TABLE_DROP   = 61; // Database, STRING -> OBJECT
  #         // Lists all the tables in a particular database.
  #         TABLE_LIST   = 62; // Database -> ARRAY
  #
  #         // * Control Operators
  #         // Calls a function on data
  #         FUNCALL  = 64; // Function(*), DATUM... -> DATUM
  #         // Executes its first argument, and returns its second argument if it
  #         // got [true] or its third argument if it got [false] (like an `if`
  #         // statement).
  #         BRANCH  = 65; // BOOL, Top, Top -> Top
  #         // Returns true if any of its arguments returns true (short-circuits).
  #         // (Like `or` in most languages.)
  #         ANY     = 66; // BOOL... -> BOOL
  #         // Returns true if all of its arguments return true (short-circuits).
  #         // (Like `and` in most languages.)
  #         ALL     = 67; // BOOL... -> BOOL
  #         // Calls its Function with each entry in the sequence
  #         // and executes the array of terms that Function returns.
  #         FOREACH = 68; // Sequence, Function(1) -> OBJECT
  #
  # ////////////////////////////////////////////////////////////////////////////////
  # ////////// Special Terms
  # ////////////////////////////////////////////////////////////////////////////////
  #
  #         // An anonymous function.  Takes an array of numbers representing
  #         // variables (see [VAR] above), and a [Term] to execute with those in
  #         // scope.  Returns a function that may be passed an array of arguments,
  #         // then executes the Term with those bound to the variable names.  The
  #         // user will never construct this directly.  We use it internally for
  #         // things like `map` which take a function.  The "arity" of a [Function] is
  #         // the number of arguments it takes.
  #         // For example, here's what `_X_.map{|x| x+2}` turns into:
  #         // Term {
  #         //   type = MAP;
  #         //   args = [_X_,
  #         //           Term {
  #         //             type = Function;
  #         //             args = [Term {
  #         //                       type = DATUM;
  #         //                       datum = Datum {
  #         //                         type = R_ARRAY;
  #         //                         r_array = [Datum { type = R_NUM; r_num = 1; }];
  #         //                       };
  #         //                     },
  #         //                     Term {
  #         //                       type = ADD;
  #         //                       args = [Term {
  #         //                                 type = VAR;
  #         //                                 args = [Term {
  #         //                                           type = DATUM;
  #         //                                           datum = Datum { type = R_NUM; r_num = 1};
  #         //                                         }];
  #         //                               },
  #         //                               Term {
  #         //                                 type = DATUM;
  #         //                                 datum = Datum { type = R_NUM; r_num = 2; };
  #         //                               }];
  #         //                     }];
  #         //           }];
  #         FUNC = 69; // ARRAY, Top -> ARRAY -> Top
  #
  #         ASC = 73;
  #         DESC = 74;
  #     };
  #     optional TermType type = 1;
  #
  #     // This is only used when type is DATUM.
  #     optional Datum datum = 2;
  #
  #     repeated Term args = 3; // Holds the positional arguments of the query.
  #     message AssocPair {
  #         optional string key = 1;
  #         optional Term val = 2;
  #     };
  #     repeated AssocPair optargs = 4; // Holds the optional arguments of the query.
  #     // (Note that the order of the optional arguments doesn't matter; think of a
  #     // Hash.)
  #
  #     extensions 10000 to 20000;
  # };
  #
  # ////////////////////////////////////////////////////////////////////////////////
  # //                                  EXAMPLE                                   //
  # ////////////////////////////////////////////////////////////////////////////////
  # //   ```ruby
  # //   r.table('tbl', {:use_outdated => true}).insert([{:id => 0}, {:id => 1}])
  # //   ```
  # // Would turn into:
  # //   Term {
  # //     type = INSERT;
  # //     args = [Term {
  # //               type = TABLE;
  # //               args = [Term {
  # //                         type = R_DATUM;
  # //                         r_datum = Datum { type = R_STR; r_str = "tbl"; };
  # //                       }];
  # //               optargs = [["use_outdated",
  # //                           Term {
  # //                             type = R_DATUM;
  # //                             r_datum = Datum { type = R_BOOL; r_bool = true; };
  # //                           }]];
  # //             },
  # //             Term {
  # //               type = R_ARRAY;
  # //               args = [Term {
  # //                         type = R_DATUM;
  # //                         r_datum = Datum { type = R_OBJECT; r_object = [["id", 0]]; };
  # //                       },
  # //                       Term {
  # //                         type = R_DATUM;
  # //                         r_datum = Datum { type = R_OBJECT; r_object = [["id", 1]]; };
  # //                       }];
  # //             }]
  # //   }
  # // And the server would reply:
  # //   Response {
  # //     type = SUCCESS_ATOM;
  # //     token = 1;
  # //     response = [Datum { type = R_OBJECT; r_object = [["inserted", 2]]; }];
  # //   }
  # // Or, if there were an error:
  # //   Response {
  # //     type = RUNTIME_ERROR;
  # //     token = 1;
  # //     response = [Datum { type = R_STR; r_str = "The table `tbl` doesn't exist!"; }];
  # //     backtrace = [Frame { type = POS; pos = 0; }, Frame { type = POS; pos = 0; }];
  # //   }

  require 'protobuf/message/message'
  require 'protobuf/message/enum'
  require 'protobuf/message/service'
  require 'protobuf/message/extend'

  class VersionDummy < ::Protobuf::Message
    defined_in __FILE__
    class Version < ::Protobuf::Enum
      defined_in __FILE__
      V0_1 = value(:V0_1, 1063369270)
    end
  end
  class Query < ::Protobuf::Message
    defined_in __FILE__
    class QueryType < ::Protobuf::Enum
      defined_in __FILE__
      START = value(:START, 1)
      CONTINUE = value(:CONTINUE, 2)
      STOP = value(:STOP, 3)
    end
    optional :QueryType, :type, 1
    optional :Term, :query, 2
    optional :int64, :token, 3
    optional :bool, :noreply, 4, :default => false
    class AssocPair < ::Protobuf::Message
      defined_in __FILE__
      optional :string, :key, 1
      optional :Term, :val, 2
    end
    repeated :AssocPair, :global_optargs, 6
  end
  class Frame < ::Protobuf::Message
    defined_in __FILE__
    class FrameType < ::Protobuf::Enum
      defined_in __FILE__
      POS = value(:POS, 1)
      OPT = value(:OPT, 2)
    end
    optional :FrameType, :type, 1
    optional :int64, :pos, 2
    optional :string, :opt, 3
  end
  class Backtrace < ::Protobuf::Message
    defined_in __FILE__
    repeated :Frame, :frames, 1
  end
  class Response < ::Protobuf::Message
    defined_in __FILE__
    class ResponseType < ::Protobuf::Enum
      defined_in __FILE__
      SUCCESS_ATOM = value(:SUCCESS_ATOM, 1)
      SUCCESS_SEQUENCE = value(:SUCCESS_SEQUENCE, 2)
      SUCCESS_PARTIAL = value(:SUCCESS_PARTIAL, 3)
      CLIENT_ERROR = value(:CLIENT_ERROR, 16)
      COMPILE_ERROR = value(:COMPILE_ERROR, 17)
      RUNTIME_ERROR = value(:RUNTIME_ERROR, 18)
    end
    optional :ResponseType, :type, 1
    optional :int64, :token, 2
    repeated :Datum, :response, 3
    optional :Backtrace, :backtrace, 4
  end
  class Datum < ::Protobuf::Message
    defined_in __FILE__
    class DatumType < ::Protobuf::Enum
      defined_in __FILE__
      R_NULL = value(:R_NULL, 1)
      R_BOOL = value(:R_BOOL, 2)
      R_NUM = value(:R_NUM, 3)
      R_STR = value(:R_STR, 4)
      R_ARRAY = value(:R_ARRAY, 5)
      R_OBJECT = value(:R_OBJECT, 6)
    end
    optional :DatumType, :type, 1
    optional :bool, :r_bool, 2
    optional :double, :r_num, 3
    optional :string, :r_str, 4
    repeated :Datum, :r_array, 5
    class AssocPair < ::Protobuf::Message
      defined_in __FILE__
      optional :string, :key, 1
      optional :Datum, :val, 2
    end
    repeated :AssocPair, :r_object, 6
    extensions 10000..20000
  end
  class Term < ::Protobuf::Message
    defined_in __FILE__
    class TermType < ::Protobuf::Enum
      defined_in __FILE__
      DATUM = value(:DATUM, 1)
      MAKE_ARRAY = value(:MAKE_ARRAY, 2)
      MAKE_OBJ = value(:MAKE_OBJ, 3)
      VAR = value(:VAR, 10)
      JAVASCRIPT = value(:JAVASCRIPT, 11)
      ERROR = value(:ERROR, 12)
      IMPLICIT_VAR = value(:IMPLICIT_VAR, 13)
      DB = value(:DB, 14)
      TABLE = value(:TABLE, 15)
      GET = value(:GET, 16)
      EQ = value(:EQ, 17)
      NE = value(:NE, 18)
      LT = value(:LT, 19)
      LE = value(:LE, 20)
      GT = value(:GT, 21)
      GE = value(:GE, 22)
      NOT = value(:NOT, 23)
      ADD = value(:ADD, 24)
      SUB = value(:SUB, 25)
      MUL = value(:MUL, 26)
      DIV = value(:DIV, 27)
      MOD = value(:MOD, 28)
      APPEND = value(:APPEND, 29)
      SLICE = value(:SLICE, 30)
      SKIP = value(:SKIP, 70)
      LIMIT = value(:LIMIT, 71)
      GETATTR = value(:GETATTR, 31)
      CONTAINS = value(:CONTAINS, 32)
      PLUCK = value(:PLUCK, 33)
      WITHOUT = value(:WITHOUT, 34)
      MERGE = value(:MERGE, 35)
      BETWEEN = value(:BETWEEN, 36)
      REDUCE = value(:REDUCE, 37)
      MAP = value(:MAP, 38)
      FILTER = value(:FILTER, 39)
      CONCATMAP = value(:CONCATMAP, 40)
      ORDERBY = value(:ORDERBY, 41)
      DISTINCT = value(:DISTINCT, 42)
      COUNT = value(:COUNT, 43)
      UNION = value(:UNION, 44)
      NTH = value(:NTH, 45)
      GROUPED_MAP_REDUCE = value(:GROUPED_MAP_REDUCE, 46)
      GROUPBY = value(:GROUPBY, 47)
      INNER_JOIN = value(:INNER_JOIN, 48)
      OUTER_JOIN = value(:OUTER_JOIN, 49)
      EQ_JOIN = value(:EQ_JOIN, 50)
      ZIP = value(:ZIP, 72)
      COERCE_TO = value(:COERCE_TO, 51)
      TYPEOF = value(:TYPEOF, 52)
      UPDATE = value(:UPDATE, 53)
      DELETE = value(:DELETE, 54)
      REPLACE = value(:REPLACE, 55)
      INSERT = value(:INSERT, 56)
      DB_CREATE = value(:DB_CREATE, 57)
      DB_DROP = value(:DB_DROP, 58)
      DB_LIST = value(:DB_LIST, 59)
      TABLE_CREATE = value(:TABLE_CREATE, 60)
      TABLE_DROP = value(:TABLE_DROP, 61)
      TABLE_LIST = value(:TABLE_LIST, 62)
      FUNCALL = value(:FUNCALL, 64)
      BRANCH = value(:BRANCH, 65)
      ANY = value(:ANY, 66)
      ALL = value(:ALL, 67)
      FOREACH = value(:FOREACH, 68)
      FUNC = value(:FUNC, 69)
      ASC = value(:ASC, 73)
      DESC = value(:DESC, 74)
    end
    optional :TermType, :type, 1
    optional :Datum, :datum, 2
    repeated :Term, :args, 3
    class AssocPair < ::Protobuf::Message
      defined_in __FILE__
      optional :string, :key, 1
      optional :Term, :val, 2
    end
    repeated :AssocPair, :optargs, 4
    extensions 10000..20000
  end

  # $f = File.open("fuzz_seed.rb", "w")

  module RethinkDB
    module Faux_Abort
      class Abort
      end
    end

    class RQL
      def self.set_default_conn c; @@default_conn = c; end
      def run(c=@@default_conn, opts=nil)
        # $f.puts "("+RPP::pp(@body)+"),"
        unbound_if !@body
        c, opts = @@default_conn, c if opts.nil? && !c.kind_of?(RethinkDB::Connection)
        opts = {} if opts.nil?
        opts = {opts => true} if opts.class != Hash
        if !c
          raise ArgumentError, "No connection specified!\n" \
          "Use `query.run(conn)` or `conn.repl(); query.run`."
        end
        c.run(@body, opts)
      end
    end

    class Cursor
      include Enumerable
      def out_of_date # :nodoc:
        @conn.conn_id != @conn_id
      end

      def inspect # :nodoc:
        preview_res = @results[0...10]
        if (@results.size > 10 || @more)
          preview_res << (dots = "..."; class << dots; def inspect; "..."; end; end; dots)
        end
        preview = preview_res.pretty_inspect[0...-1]
        state = @run ? "(exhausted)" : "(enumerable)"
        extra = out_of_date ? " (Connection #{@conn.inspect} reset!)" : ""
        "#<RethinkDB::Cursor:#{self.object_id} #{state}#{extra}: #{RPP.pp(@msg)}" +
          (@run ? "" : "\n#{preview}") + ">"
      end

      def initialize(results, msg, connection, token, more = true) # :nodoc:
        @more = more
        @results = results
        @msg = msg
        @run = false
        @conn_id = connection.conn_id
        @conn = connection
        @token = token
      end

      def each (&block) # :nodoc:
        raise RqlRuntimeError, "Can only iterate over Query_Results once!" if @run
        @run = true
        raise RqlRuntimeError, "Connection has been reset!" if out_of_date
        while true
          @results.each(&block)
          return self if !@more
          q = Query.new
          q.type = Query::QueryType::CONTINUE
          q.token = @token
          res = @conn.run_internal q
          @results = Shim.response_to_native(res, @msg)
          if res.type == Response::ResponseType::SUCCESS_SEQUENCE
            @more = false
          end
        end
      end
    end

    class Connection
      def repl; RQL.set_default_conn self; end

      def initialize(host='localhost', port=28015, default_db=nil)
        begin
          @abort_module = ::IRB
        rescue NameError => e
          @abort_module = Faux_Abort
        end
        @@last = self
        @host = host
        @port = port
        @default_opts = default_db ? {:db => RQL.new.db(default_db)} : {}
        @conn_id = 0
        reconnect
      end
      attr_reader :default_db, :conn_id

      @@token_cnt = 0
      def run_internal q
        dispatch q
        wait q.token
      end
      def run(msg, opts)
        q = Query.new
        q.type = Query::QueryType::START
        q.query = msg
        q.token = @@token_cnt += 1

        @default_opts.merge(opts).each {|k,v|
          ap = Query::AssocPair.new
          ap.key = k.to_s
          ap.val = v.to_pb
          q.global_optargs << ap
        }

        res = run_internal q
        if res.type == Response::ResponseType::SUCCESS_PARTIAL
          Cursor.new(Shim.response_to_native(res, msg), msg, self, q.token, true)
        elsif res.type == Response::ResponseType::SUCCESS_SEQUENCE
          Cursor.new(Shim.response_to_native(res, msg), msg, self, q.token, false)
        else
          Shim.response_to_native(res, msg)
        end
      end

      def send packet
        @socket.write(packet)
      end

      def dispatch msg
        PP.pp msg if $DEBUG
        payload = msg.serialize_to_string
        #File.open('sexp_payloads.txt', 'a') {|f| f.write(payload.inspect+"\n")}
        send([payload.length].pack('L<') + payload)
        return msg.token
      end

      def wait token
        begin
          res = nil
          raise RqlRuntimeError, "Connection closed by server!" if not @listener
          @mutex.synchronize do
            (@waiters[token] = ConditionVariable.new).wait(@mutex) if not @data[token]
            res = @data.delete token if @data[token]
          end
          raise RqlRuntimeError, "Connection closed by server!" if !@listener or !res
          return res
        rescue @abort_module::Abort => e
          print "\nAborting query and reconnecting...\n"
          reconnect
          raise e
        end
      end

      # Change the default database of a connection.
      def use(new_default_db)
        @default_opts[:db] = RQL.new.db(new_default_db)
      end

      def inspect
        db = @default_opts[:db] || RQL.new.db('test')
        properties = "(#{@host}:#{@port}) (Default DB: #{db.inspect})"
        state = @listener ? "(listening)" : "(closed)"
        "#<RethinkDB::Connection:#{self.object_id} #{properties} #{state}>"
      end

      @@last = nil
      @@magic_number = 0x3f61ba36

      def debug_socket; @socket; end

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

      def close
        @listener.terminate if @listener
        @listener = nil
        @socket.close
        @socket = nil
      end

      def self.last
        return @@last if @@last
        raise RqlRuntimeError, "No last connection.  Use RethinkDB::Connection.new."
      end

      def start_listener
        class << @socket
          def read_exn(len)
            buf = read len
            if !buf or buf.length != len
              raise RqlRuntimeError, "Connection closed by server."
            end
            return buf
          end
        end
        @socket.write([@@magic_number].pack('L<'))
        @listener.terminate if @listener
        @listener = Thread.new do
          loop do
            begin
              response_length = @socket.read_exn(4).unpack('L<')[0]
              response = @socket.read_exn(response_length)
            rescue RqlRuntimeError => e
              @mutex.synchronize do
                @listener = nil
                @waiters.each {|kv| kv[1].signal}
              end
              Thread.current.terminate
              abort("unreachable")
            end
            #TODO: Recovery
            begin
              protob = Response.new.parse_from_string(response)
            rescue
              raise RqlRuntimeError, "Bad Protobuf #{response}, server is buggy."
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
    end
  end

  module RethinkDB
    module Shim
      def self.datum_to_native d
        raise RqlRuntimeError, "SHENANIGANS" if d.class != Datum
        dt = Datum::DatumType
        case d.type
        when dt::R_NUM then d.r_num
        when dt::R_STR then d.r_str
        when dt::R_BOOL then d.r_bool
        when dt::R_NULL then nil
        when dt::R_ARRAY then d.r_array.map{|d2| datum_to_native d2}
        when dt::R_OBJECT then Hash[d.r_object.map{|x| [x.key, datum_to_native(x.val)]}]
        else raise RqlRuntimeError, "Unimplemented."
        end
      end

      def self.response_to_native(r, orig_term)
        rt = Response::ResponseType
        if r.backtrace
          bt = r.backtrace.frames.map {|x|
            x.type == Frame::FrameType::POS ? x.pos : x.opt
          }
        else
          bt = []
        end

        begin
          case r.type
          when rt::SUCCESS_ATOM then datum_to_native(r.response[0])
          when rt::SUCCESS_PARTIAL then r.response.map{|d| datum_to_native(d)}
          when rt::SUCCESS_SEQUENCE then r.response.map{|d| datum_to_native(d)}
          when rt::RUNTIME_ERROR then
            raise RqlRuntimeError, "#{r.response[0].r_str}"
          when rt::COMPILE_ERROR then # TODO: remove?
            raise RqlCompileError, "#{r.response[0].r_str}"
          when rt::CLIENT_ERROR then
            raise RqlDriverError, "#{r.response[0].r_str}"
          else raise RqlRuntimeError, "Unexpected response: #{r.inspect}"
          end
        rescue RqlError => e
          term = orig_term.dup
          term.bt_tag(bt)
          $t = term
          raise e.class, "#{e.message}\nBacktrace:\n#{RPP.pp(term)}"
        end
      end

      def self.native_to_datum_term x
        dt = Datum::DatumType
        d = Datum.new
        case x.class.hash
        when Fixnum.hash     then d.type = dt::R_NUM;  d.r_num = x
        when Float.hash      then d.type = dt::R_NUM;  d.r_num = x
        when Bignum.hash     then d.type = dt::R_NUM;  d.r_num = x
        when String.hash     then d.type = dt::R_STR;  d.r_str = x
        when Symbol.hash     then d.type = dt::R_STR;  d.r_str = x.to_s
        when TrueClass.hash  then d.type = dt::R_BOOL; d.r_bool = x
        when FalseClass.hash then d.type = dt::R_BOOL; d.r_bool = x
        when NilClass.hash   then d.type = dt::R_NULL
        else raise RqlRuntimeError, "UNREACHABLE"
        end
        t = Term.new
        t.type = Term::TermType::DATUM
        t.datum = d
        return t
      end
    end

    class RQL
      def to_pb; @body; end

      def expr(x)
        unbound_if @body
        return x if x.class == RQL
        datum_types = [Fixnum, Float, Bignum, String, Symbol, TrueClass, FalseClass, NilClass]
        if datum_types.map{|y| y.hash}.include? x.class.hash
          return RQL.new(Shim.native_to_datum_term(x))
        end

        t = Term.new
        case x.class.hash
        when Array.hash
          t.type = Term::TermType::MAKE_ARRAY
          t.args = x.map{|y| expr(y).to_pb}
        when Hash.hash
          t.type = Term::TermType::MAKE_OBJ
          t.optargs = x.map{|k,v| ap = Term::AssocPair.new;
            ap.key = k.to_s; ap.val = expr(v).to_pb; ap}
        when Proc.hash
          t = RQL.new.new_func(&x).to_pb
        else raise RqlDriverError, "r.expr can't handle #{x.inspect} of type #{x.class}"
        end

        return RQL.new(t)
      end

      def coerce(other)
        [RQL.new.expr(other), self]
      end
    end
  end

  module RethinkDB
    class RQL
      @@gensym_cnt = 0
      def new_func(&b)
        args = (0...b.arity).map{@@gensym_cnt += 1}
        body = b.call(*(args.map{|i| RQL.new.var i}))
        RQL.new.func(args, body)
      end

      @@special_optargs = {
        :replace => :non_atomic, :update => :non_atomic, :insert => :upsert
      }
      @@opt_off = {
        :reduce => -1, :between => -1, :grouped_map_reduce => -1,
        :table => -1, :table_create => -1
      }
      @@rewrites = {
        :< => :lt, :<= => :le, :> => :gt, :>= => :ge,
        :+ => :add, :- => :sub, :* => :mul, :/ => :div, :% => :mod,
        :"|" => :any, :or => :any,
        :"&" => :all, :and => :all,
        :order_by => :orderby,
        :group_by => :groupby,
        :concat_map => :concatmap,
        :for_each => :foreach,
        :js => :javascript,
        :type_of => :typeof
      }
      def method_missing(m, *a, &b)
        bitop = [:"|", :"&"].include?(m) ? [m, a, b] : nil
        if [:<, :<=, :>, :>=, :+, :-, :*, :/, :%].include?(m)
          a.each {|arg|
            if arg.class == RQL && arg.bitop
              err = "Calling #{m} on result of infix bitwise operator:\n" +
                "#{arg.inspect}.\n" +
                "This is almost always a precedence error.\n" +
                "Note that `a < b | b < c` <==> `a < (b | b) < c`.\n" +
                "If you really want this behavior, use `.or` or `.and` instead."
              raise ArgumentError, err
            end
          }
        end

        m = @@rewrites[m] || m
        termtype = Term::TermType.values[m.to_s.upcase.to_sym]
        unbound_if(!termtype, m)

        if (opt_name = @@special_optargs[m])
          a = optarg_jiggle(a, opt_name)
          opt_offset = -1
        end
        if (opt_offset ||= @@opt_off[m])
          optargs = a.delete_at(opt_offset) if a[opt_offset].class == Hash
        end

        args = (@body ? [self] : []) + a + (b ? [new_func(&b)] : [])

        t = Term.new
        t.type = termtype
        t.args = args.map{|x| RQL.new.expr(x).to_pb}
        t.optargs = (optargs || {}).map {|k,v|
          ap = Term::AssocPair.new
          ap.key = k.to_s
          ap.val = RQL.new.expr(v).to_pb
          ap
        }
        return RQL.new(t, bitop)
      end

      def group_by(*a, &b)
        a = [self] + a if @body
        RQL.new.method_missing(:group_by, a[0], a[1..-2], a[-1], &b)
      end
      def groupby(*a, &b); group_by(*a, &b); end

      def optarg_jiggle(args, optarg)
        if (ind = args.map{|x| x.class == Symbol ? x : nil}.index(optarg))
          args << {args.delete_at(ind) => true}
        else
          args << {}
        end
        return args
      end

      def connect(*args)
        unbound_if @body
        Connection.new(*args)
      end

      def avg(attr)
        unbound_if @body
        {:AVG => attr}
      end
      def sum(attr)
        unbound_if @body
        {:SUM => attr}
      end
      def count(*a, &b)
        !@body && a == [] ? {:COUNT => true} : super(*a, &b)
      end

      def reduce(*a, &b)
        a = a[1..-2] + [{:base => a[-1]}] if a.size + (@body ? 1 : 0) == 2
        super(*a, &b)
      end

      def grouped_map_reduce(*a, &b)
        a << {:base => a.delete_at(-2)} if a.size >= 2 && a[-2].class != Proc
        super(*a, &b)
      end

      def between(l=nil, r=nil)
        super(Hash[(l ? [['left_bound', l]] : []) + (r ? [['right_bound', r]] : [])])
      end

      def -@; RQL.new.sub(0, self); end

      def [](ind)
        if ind.class == Fixnum
          return nth(ind)
        elsif ind.class == Symbol || ind.class == String
          return getattr(ind)
        elsif ind.class == Range
          if ind.end == 0 && ind.exclude_end?
            raise ArgumentError, "Cannot slice to an excluded end of 0."
          end
          return slice(ind.begin, ind.end - (ind.exclude_end? ? 1 : 0))
        end
        raise ArgumentError, "[] cannot handle #{ind.inspect} of type #{ind.class}."
      end

      def ==(rhs)
        raise ArgumentError,"
      Cannot use inline ==/!= with RQL queries, use .eq() instead if
      you want a query that does equality comparison.

      If you need to see whether two queries are the same, compare
      their protobufs like: `query1.to_pb == query2.to_pb`."
      end


      def do(*args, &b)
        a = (@body ? [self] : []) + args.dup
        if a == [] && !b
          raise RqlDriverError, "Expected 1 or more argument(s) but found 0."
        end
        RQL.new.funcall(*((b ? [new_func(&b)] : [a.pop]) + a))
      end

      def row
        unbound_if @body
        raise NoMethodError, ("Sorry, r.row is not available in the ruby driver.  " +
                              "Use blocks instead.")
      end
    end
  end

  module RethinkDB
    module RPP
      def self.sanitize_context context
        if __FILE__ =~ /^(.*\/)[^\/]+.rb$/
          prefix = $1;
          context.reject{|x| x =~ /^#{prefix}/}
        else
          context
        end
      end

      def self.pp_int_optargs(q, optargs, pre_dot = false)
        q.text("r(") if pre_dot
        q.group(1, "{", "}") {
          optargs.each_with_index {|optarg, i|
            if i != 0
              q.text(",")
              q.breakable
            end
            q.text(":#{optarg.key} =>")
            q.nest(2) {
              q.breakable
              pp_int(q, optarg.val)
            }
          }
        }
        q.text(")") if pre_dot
      end

      def self.pp_int_args(q, args, pre_dot = false)
        q.text("r(") if pre_dot
        q.group(1, "[", "]") {
          args.each_with_index {|arg, i|
            if i != 0
              q.text(",")
              q.breakable
            end
            pp_int(q, arg)
          }
        }
        q.text(")") if pre_dot
      end

      def self.pp_int_datum(q, dat, pre_dot)
        q.text("r(") if pre_dot
        q.text(Shim.datum_to_native(dat).inspect)
        q.text(")") if pre_dot
      end

      def self.pp_int_func(q, func)
        func_args = func.args[0].args.map{|x| x.datum.r_num}
        func_body = func.args[1]
        q.text(" ")
        q.group(0, "{", "}") {
          if func_args != []
            q.text(func_args.map{|x| :"var_#{x}"}.inspect.gsub(/\[|\]/,"|").gsub(":",""))
          end
          q.nest(2) {
            q.breakable
            pp_int(q, func_body)
          }
          q.breakable('')
        }
      end

      def self.can_prefix (name, args)
        return false if name == "db" || name == "funcall"
        return false if args.size == 1 && args[0].type == Term::TermType::DATUM
        return true
      end
      def self.pp_int(q, term, pre_dot=false)
        q.text("\x7", 0) if term.is_error
        @@context = term.context if term.is_error

        if term.type == Term::TermType::DATUM
          res = pp_int_datum(q, term.datum, pre_dot)
          q.text("\x7", 0) if term.is_error
          return res
        end

        if term.type == Term::TermType::VAR
          q.text("var_")
          res = pp_int_datum(q, term.args[0].datum, false)
          q.text("\x7", 0) if term.is_error
          return res
        elsif term.type == Term::TermType::FUNC
          q.text("r(") if pre_dot
          q.text("lambda")
          res = pp_int_func(q, term)
          q.text(")") if pre_dot
          q.text("\x7", 0) if term.is_error
          return res
        elsif term.type == Term::TermType::MAKE_OBJ
          res = pp_int_optargs(q, term.optargs, pre_dot)
          q.text("\x7", 0) if term.is_error
          return res
        elsif term.type == Term::TermType::MAKE_ARRAY
          res = pp_int_args(q, term.args, pre_dot)
          q.text("\x7", 0) if term.is_error
          return res
        end

        name = term.type.to_s.downcase
        args = term.args.dup
        optargs = term.optargs.dup

        if can_prefix(name, args) && first_arg = args.shift
          pp_int(q, first_arg, true)
        else
          q.text("r")
        end
        if name == "getattr"
          argstart, argstop = "[", "]"
        else
          q.text(".")
          q.text(name)
          argstart, argstop = "(", ")"
        end

        func = args[-1] && args[-1].type == Term::TermType::FUNC && args.pop

        if args != [] || optargs != []
          q.group(0, argstart, argstop) {
            pushed = nil
            q.nest(2) {
              args.each {|arg|
                if !pushed
                  pushed = true
                  q.breakable('')
                else
                  q.text(",")
                  q.breakable
                end
                pp_int(q, arg)
              }
              if optargs != []
                if pushed
                  q.text(",")
                  q.breakable
                end
                pp_int_optargs(q, optargs)
              end
              if func && name == "grouped_map_reduce"
                q.text(",")
                q.breakable
                q.text("lambda")
                pp_int_func(q, func)
                func = nil
              end
            }
            q.breakable('')
          }
        end

        pp_int_func(q, func) if func
        q.text("\x7", 0) if term.is_error
      end

      def self.pp term
        begin
          @@context = nil
          q = PrettyPrint.new
          pp_int(q, term, true)
          q.flush

          in_bt = false
          q.output.split("\n").map {|line|
            line = line.gsub(/^ */) {|x| x+"\x7"} if in_bt
            arr = line.split("\x7")
            if arr[1]
              in_bt = !(arr[2] || (line[-1] == 0x7))
              [arr.join(""), " "*arr[0].size + "^"*arr[1].size]
            else
              line
            end
          }.flatten.join("\n") +
            (@@context ?
             "\nErronious_Portion_Constructed:\n" +
             "#{@@context.map{|x| "\tfrom "+x}.join("\n")}" +
             "\nCalled:" : "")
        rescue Exception => e
          raise e
          "AN ERROR OCCURED DURING PRETTY-PRINTING:\n#{e.inspect}\n" +
            "FALLING BACK TO PROTOBUF PRETTY-PRINTER.\n#{@term.inspect}"
        end
      end
    end
  end

  module RethinkDB
    class RqlError < RuntimeError; end
    class RqlRuntimeError < RqlError; end
    class RqlDriverError < RqlError; end
    class RqlCompileError < RqlError; end

    # class Error < StandardError
    #   def initialize(msg, bt)
    #     @msg = msg
    #     @bt = bt
    #     @pp_query_bt = "UNIMPLEMENTED #{bt.inspect}"
    #     super inspect
    #   end
    #   def inspect
    #     "#{@msg}\n#{@pp_query_bt}"
    #   end
    #   attr_accessor :msg, :bt, :pp_query_bt
    # end

    # class Connection_Error < Error; end

    # class Compile_Error < Error; end
    # class Malformed_Protobuf_Error < Compile_Error; end
    # class Malformed_Query_Error < Compile_Error; end

    # class Runtime_Error < Error; end
    # class Data_Error < Runtime_Error; end
    # class Type_Error < Data_Error; end
    # class Resource_Error < Runtime_Error; end
  end


  class Term
    attr_accessor :context
    attr_accessor :is_error

    def bt_tag bt
      @is_error = true
      begin
        return if bt == []
        frame, sub_bt = bt[0], bt [1..-1]
        if frame.class == String
          optargs.each {|optarg|
            if optarg.key == frame
              @is_error = false
              optarg.val.bt_tag(sub_bt)
            end
          }
        else
          @is_error = false
          args[frame].bt_tag(sub_bt)
        end
      rescue StandardError => e
        @is_error = true
        $stderr.puts "ERROR: Failed to parse backtrace (we'll take our best guess)."
      end
    end
  end

  module RethinkDB
    module Shortcuts
      def r(*args)
        args == [] ? RQL.new : RQL.new.expr(*args)
      end
    end

    module Utils
      def get_mname(i = 0)
        caller[i]=~/`(.*?)'/
        $1
      end
      def unbound_if (x, name = nil)
        name = get_mname(1) if not name
        raise NoMethodError, "undefined method `#{name}'" if x
      end
    end

    class RQL
      include Utils

      attr_accessor :body, :bitop

      def initialize(body = nil, bitop = nil)
        @body = body
        @bitop = bitop
        @body.context = RPP.sanitize_context caller if @body
      end

      def pp
        unbound_if !@body
        RethinkDB::RPP.pp(@body)
      end
      def inspect
        @body ? pp : super
      end
    end
  end

  class Runner
    def initialize(host, port)
      extend RethinkDB::Shortcuts
      @c = RethinkDB::Connection.new(host, port, 'test')
    end
    def db_list
      r.db_list.run(@c)
    end
    def table_list(db)
      r.db(db).table_list.run(@c)
    end
    def rows(db, tbl)
      r.db(db).table(tbl).run(@c)
    end
    def db_create(db)
      r.db_create(db).run(@c) if db != 'test'
    end
    def table_create(db, tbl, key)
      r.db(db).table_create(tbl, {:primary_key => key}).run(@c)
    end
    def insert_rows(db, tbl, rows)
      r.db(db).table(tbl).insert(rows).run(@c)
    end
  end
end

require 'time'
def pr x
  puts(Time.now.iso8601 + ' ' + x.to_s)
end
def vpr x
  pr x if $verbose
end

def export(runner, dir)
  Dir.mkdir dir
  Dir.chdir(dir) {
    runner.db_list.each {|db|
      Dir.mkdir db
      Dir.chdir(db) {
        pr "Exporting database #{db}..."
        runner.table_list(db).each {|tbl|
          pr "Exporting table #{db+'.'+tbl}..."
          File.open(tbl, "w") {|f|
            runner.rows(db, tbl).each {|row|
              f.puts row.to_json
            }
          }
        }
      }
    }
  }
end

$row_batch_size = 100
$char_batch_size = 3000
def import(runner, dir)
  Dir.chdir(dir) {
    dbs = Dir.open('.') {|d| d.grep(/^[^.]/)}
    dbs.each {|db|
      Dir.chdir(db) {
        pr "Importing database #{db}..."
        runner.db_create(db)
        tables = Dir.open('.') {|d| d.grep(/^[^.]/)}
        tables.each {|tbl|
          tblspec = db+'.'+tbl
          key = $keys[tblspec] || 'id'
          pr "Importing table #{tblspec} with primary key #{key}..."
          runner.table_create(db, tbl, key)
          File.open(tbl) {|f|
            rows_inserted = 0
            begin
              total_rows = `wc -l #{tbl}`.to_i
            rescue Exception => e
              total_rows = "? (#{e.inspect})"
            end
            vpr "Preparing to insert #{total_rows} rows..."

            charcount = 0
            rows = []
            f.each_line {|l|
              charcount += l.size
              rows << JSON.parse(l)
              if rows.size >= $row_batch_size || charcount >= $char_batch_size
                rows_inserted += rows.size
                vpr "Inserting #{rows.size} rows (#{rows_inserted}/#{total_rows})..."
                runner.insert_rows(db, tbl, rows)
                charcount = 0
                rows = []
              end
            }
            rows_inserted += rows.size
            vpr "Inserting #{rows.size} rows (#{rows_inserted}/#{total_rows})..."
            runner.insert_rows(db, tbl, rows) if rows.size > 0
          }
        }
      }
    }
  }
end

require 'optparse'

$host = 'localhost'
$port = 28015
$verbose = false
$opt_parser = OptionParser.new {|opts|
  opts.banner = "Usage: import_export.rb [options]"
  opts.on("-h", "--host HOST",
          "host to connect to (default #{$host})") {|h| $host = h}
  opts.on("-p", "--port PORT", Integer,
          "port to connect on (default #{$port})") {|p| $port = p}
  opts.on("-e", "--export",
          "export data from RethinkDB into new directory") {|e| $export = e}
  opts.on("-i", "--import [DIR]",
          "import data into RethinkDB from directory DIR") {|i| $import = i || true}
  opts.on("-k", "--primary-keys JSON_SPEC",
          "when importing data from old RethinkDB versions, you",
          "must specify all non-`id` primary keys as follows:",
          "'{\"db_name.table_name\": \"key_name\"}'") {|k| $keys_str = k}
  opts.on("-v", "--verbose", "be verbose") {|v| $verbose = v}
}
def test_class(x, cls)
  raise ArgumentError, "#{x} is of type #{x.class} instead of #{cls}" if x.class != cls
end
begin
  $opt_parser.parse!(ARGV)
  if ($export && $import) || !($export || $import)
    raise OptionParser::InvalidArgument, "must specify either --export OR --import"
  end
  begin
    $keys = $keys_str ? JSON.parse($keys_str) : {}
    test_class($keys, Hash)
    $keys.each {|k,v|
      test_class(k, String)
      test_class(v, String)
    }
  rescue Exception => e
    str = "key specifier `#{$keys_str}` is not a valid String:String JSON object (#{e})"
    raise OptionParser::InvalidArgument, str
  end
rescue OptionParser::ParseError => e
  puts "OPTION PARSING ERROR: #{e}"
  $opt_parser.parse!(["--help"])
  exit 1
end


if $import == true
  possible_dirs = Dir.open("."){|d| d.grep(/^rethinkdb_export_/)}
  if possible_dirs.size == 1
    $import = possible_dirs[0]
    pr "No directory specified, defaulting to `#{$import}`."
  else
    pr "ERROR: Multiple import candidates, please specify directory:"
    possible_dirs.each{|d| pr "  "+d}
    pr "Usage: import_export.rb [options] --import <directory>"
    exit 1
  end
end

if $export
  $export = "rethinkdb_export_#{Time.now.iso8601}_#{$$}"
end

if $import
  pr "Importing from #{$import}..."
else
  pr "Exporting to #{$export}..."
end

pr "Connecting to #{$host}:#{$port}..."

$qls = [Query_Language_1, Query_Language_2]
$exceptions = {}
$qls.each {|ql|
  begin
    $runner = ql::Runner.new($host, $port)
    $runner.db_list
    pr "Connected with query language #{ql}..."
    break;
  rescue Exception => e
    $exceptions[ql] = e
    $runner = nil
  end
}
if $runner == nil
  pr "Could not connect to #{$host}:#{$port} with any protocol:"
  puts $exceptions.pretty_inspect
  exit 1
end

if $import
  import($runner, $import)
else
  export($runner, $export)
end

pr "Done!"

#export('localhost', 28015+32600)
