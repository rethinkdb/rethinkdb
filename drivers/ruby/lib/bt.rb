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
