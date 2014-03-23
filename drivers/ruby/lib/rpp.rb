module RethinkDB
  require 'pp'
  require 'prettyprint'

  module RPP
    @@termtype_to_str = Hash[
                             Term::TermType.constants.map{|x| [Term::TermType.const_get(x), x.to_s]}
                            ]
    @@regex = if __FILE__ =~ /^(.*\/)[^\/]+.rb$/ then /^#{$1}/ else nil end

    def self.bt_consume(bt, el)
      (bt && bt[0] == el) ? bt[1..-1] : nil
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
      PP.pp [:func, func.to_json, bt]
      func_args = func[:a][0][:a].map{|x| x.to_pb[:d]}
      func_body = func[:a][1]
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
    end

    def self.can_prefix (name, args)
      return false if name == "db" || name == "funcall"
      return false if args.size == 1 && args[0][:t] == Term::TermType::DATUM
      return true
    end
    def self.pp_int(q, term, bt, pre_dot=false)
      q.text("\x7", 0) if bt == []

      term = term.to_pb if term.class == RQL
      PP.pp [:pp_int, term.to_json, bt]
      if term[:t] == Term::TermType::DATUM
        res = pp_int_datum(q, term[:d], pre_dot)
        q.text("\x7", 0) if bt == []
        return res
      end

      if term[:t] == Term::TermType::VAR
        q.text("var_")
        res = pp_int_datum(q, term[:a][0][:d], false)
        q.text("\x7", 0) if bt == []
        return res
      elsif term[:t] == Term::TermType::FUNC
        q.text("r(") if pre_dot
        q.text("lambda")
        res = pp_int_func(q, term, bt)
        q.text(")") if pre_dot
        q.text("\x7", 0) if bt == []
        return res
      elsif term[:t] == Term::TermType::MAKE_OBJ
        res = pp_int_optargs(q, term[:o], bt, pre_dot)
        q.text("\x7", 0) if bt == []
        return res
      elsif term[:t] == Term::TermType::MAKE_ARRAY
        res = pp_int_args(q, term[:a], bt, pre_dot)
        q.text("\x7", 0) if bt == []
        return res
      end

      name = @@termtype_to_str[term[:t]].downcase
      args = term[:a].dup
      optargs = (term[:o] || {}).dup

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

      if args[-1] && args[-1][:t] == Term::TermType::FUNC
        func = args[-1]
        func_bt = bt_consume(bt, args.size() - 1 + arg_offset)
        PP.pp [:func_bt, bt, args.size() - 1 + arg_offset, func_bt]
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
              # PP.pp [:int, arg.to_json, bt_consume(bt, index)]
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
      PP.pp bt
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
      # rescue Exception => e
      #   raise e.class, "AN ERROR OCCURED DURING PRETTY-PRINTING:\n#{e.inspect}\n" +
      #     "FALLING BACK TO GENERIC PRINTER.\n#{@term.inspect}"
      end
    end
  end
end
