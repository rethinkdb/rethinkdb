require 'pp'
require 'prettyprint'

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

    def self.pp_int_optargs(q, optargs)
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
      return false if name == "db"
      return false if name == "table" && args.size == 1
      return true
    end
    def self.pp_int(q, term, pre_dot=false)
      q.text("\0x43", 0) if term.is_error
      return pp_int_datum(q, term.datum, pre_dot) if term.type == Term2::TermType::DATUM

      if term.type == Term2::TermType::VAR
        q.text("var_")
        return pp_int_datum(q, term.args[0].datum, false)
      elsif term.type == Term2::TermType::FUNC
        q.text("lambda")
        return pp_int_func(q, term)
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

      func = args[-1] && args[-1].type == Term2::TermType::FUNC && args.pop

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
      q.text("\0x43", 0) if term.is_error
    end

    def self.pp term
      q = PrettyPrint.new
      pp_int(q, term)
      q.flush

      in_bt = false
      q.output.split("\n").map {|line|
        line = line.gsub(/^ +/) {|x| x+"\0x43"} if in_bt
        arr = line.split("\0x43")
        if arr[1]
          in_bt = !arr[2]
          [arr.join(""), " "*arr[0].size + "^"*arr[1].size]
        else line
        end
      }.flatten.join("\n")
    end
  end
end
