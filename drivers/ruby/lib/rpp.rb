require 'prettyprint'

module RethinkDB
  module RPP
    @@termtype_to_str =
      Hash[Term::TermType.constants.map{|x| [Term::TermType.const_get(x), x.to_s]}]
    @@regex = if __FILE__ =~ /^(.*\/)[^\/]+.rb$/ then /^#{$1}/ else nil end

    def self.bt_consume(bt, el)
      (bt && bt[0] == el) ? bt[1..-1] : nil
    end

    def self.pp_pseudotype(q, pt, bt)
      if not pt.has_key?('$reql_type$')
        return false
      end

      case pt['$reql_type$']
      when 'TIME' then
        if not (pt.has_key?('epoch_time') and pt.has_key?('timezone'))
          return false
        end
        t = Time.at(pt['epoch_time'])
        tz = pt['timezone']
        t = (tz && tz != "" && tz != "Z") ? t.getlocal(tz) : t.utc
        q.text("r.expr(#{t.inspect})")
      when 'BINARY' then
        if not (pt.has_key?('data'))
          return false
        end
        q.text("<data>")
      else return false
      end

      return true
    end

    def self.pp_int_optargs(q, optargs, bt, pre_dot = false)
      q.text("r(") if pre_dot
      q.group(1, "{", "}") {
        optargs.to_a.each_with_index {|optarg, i|
          if i != 0
            q.text(",")
            q.breakable
          end
          q.text("#{optarg[0].inspect} =>")
          q.nest(2) {
            q.breakable
            pp_int(q, optarg[1], bt_consume(bt, optarg[0]))
          }
        }
      }
      q.text(")") if pre_dot
    end

    def self.pp_int_args(q, args, bt, pre_dot = false)
      q.text("r(") if pre_dot
      q.group(1, "[", "]") {
        args.each_with_index {|arg, i|
          if i != 0
            q.text(",")
            q.breakable
          end
          pp_int(q, arg, bt_consume(bt, i))
        }
      }
      q.text(")") if pre_dot
    end

    def self.pp_int_datum(q, dat, pre_dot)
      q.text("r(") if pre_dot
      q.text(dat.inspect)
      q.text(")") if pre_dot
    end

    def self.pp_int_func(q, func, bt)
      begin
        func_args = func[1][0][1].map{|x| x.to_pb}
        func_body = func[1][1]
        q.text(" ")
        q.group(0, "{", "}") {
          if func_args != []
            q.text(func_args.map{|x| :"var_#{x}"}.inspect.gsub(/\[|\]/,"|").gsub(":",""))
          end
          q.nest(2) {
            q.breakable
            pp_int(q, func_body, bt_consume(bt, 1))
          }
          q.breakable('')
        }
      rescue StandardError => e
        q.text(" {#<unprintable function:`#{func}`>}")
      end
    end

    def self.can_prefix (name, args)
      if (name == "table" ||
          name == "table_drop" ||
          name == "table_create") &&
          (!args.is_a?(Array) ||
               !args[0].is_a?(Array) ||
               args[0][0] != Term::TermType::DB)
        return false
      else
        return !["db", "db_create", "db_drop", "json", "funcall", "args", "branch", "http",
          "binary", "javascript", "random", "time", "iso8601", "epoch_time", "now",
          "geojson", "point", "circle", "line", "polygon", "asc", "desc", "literal",
          "range"].include?(name)
      end
    end
    def self.pp_int(q, term, bt, pre_dot=false)
      q.text("\x7", 0) if bt == []

      term = term.to_pb if term.class == RQL
      if term.class != Array
        if term.class == Hash
          if not pp_pseudotype(q, term, bt)
            pp_int_optargs(q, term, bt, pre_dot)
          end
        else
          pp_int_datum(q, term, pre_dot)
        end
        q.text("\x7", 0) if bt == []
        return
      end

      type = term[0]
      args = (term[1] || []).dup
      optargs = (term[2] || {}).dup
      if type == Term::TermType::VAR
        q.text("var_")
        res = pp_int_datum(q, args[0], false)
        q.text("\x7", 0) if bt == []
        return
      elsif type == Term::TermType::FUNC
        q.text("r(") if pre_dot
        q.text("lambda")
        pp_int_func(q, term, bt)
        q.text(")") if pre_dot
        q.text("\x7", 0) if bt == []
        return
      elsif type == Term::TermType::MAKE_ARRAY
        pp_int_args(q, args, bt, pre_dot)
        q.text("\x7", 0) if bt == []
        return
      elsif type == Term::TermType::FUNCALL
        func = (args[0][0] == Term::TermType::FUNC) ? args[0] : nil
        if args.size == 2
          pp_int(q, args[1], bt_consume(bt, 1), pre_dot)
          q.text(".do")
          if !func
            q.text("(")
            pp_int(q, args[0], bt_consume(bt, 0)) if !func
            q.text(")")
          end
        else
          q.text("r.do(")
          (1...args.size).each {|i|
            q.text(", ") if i != 1
            pp_int(q, args[i], bt_consume(bt, i))
          }
          if !func
            q.text(", ")
            pp_int(q, args[0], bt_consume(bt, 0))
          end
          q.text(")")
        end
        pp_int_func(q, args[0], bt_consume(bt, 0)) if func
        return
      end

      name = @@termtype_to_str[type].downcase

      if can_prefix(name, args) && first_arg = args.shift
        pp_int(q, first_arg, bt_consume(bt, 0), true)
        arg_offset = 1
      else
        q.text("r")
        arg_offset = 0
      end
      if name == "getattr"
        argstart, argstop = "[", "]"
      else
        q.text(".")
        q.text(name)
        argstart, argstop = "(", ")"
      end

      if args[-1] && args[-1].class == Array && args[-1][0] == Term::TermType::FUNC
        func_bt = bt_consume(bt, args.size() - 1 + arg_offset)
        func = args.pop
      end

      if args != [] || optargs != {}
        q.group(0, argstart, argstop) {
          pushed = nil
          q.nest(2) {
            args.each_with_index {|arg, index|
              if !pushed
                pushed = true
                q.breakable('')
              else
                q.text(",")
                q.breakable
              end
              pp_int(q, arg, bt_consume(bt, index + arg_offset))
            }
            if optargs != {}
              if pushed
                q.text(",")
                q.breakable
              end
              pp_int_optargs(q, optargs, bt)
            end
          }
          q.breakable('')
        }
      end

      pp_int_func(q, func, func_bt) if func
      q.text("\x7", 0) if bt == []
    end

    def self.pp(term, bt=nil)
      begin
        q = PrettyPrint.new
        pp_int(q, term, bt, true)
        q.flush

        in_bt = false
        q.output.split("\n").map {|line|
          line = line.gsub(/^ */) {|x| x+"\x7"} if in_bt
          arr = line.split("\x7")
          if arr[1]
            in_bt = !(arr[2] || (line[-1] == "\x7"))
            [arr.join(""), " "*arr[0].size + "^"*arr[1].size]
          else
            line
          end
        }.flatten.join("\n")
       rescue Exception => e
        "AN ERROR OCCURED DURING PRETTY-PRINTING:\n#{e.inspect}\n" +
          "FALLING BACK TO GENERIC PRINTER.\n#{term.inspect}"
      end
    end
  end
end
