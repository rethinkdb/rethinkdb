require 'pp'

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
      q.text(Shim.datum_to_native(dat).to_s)
      q.text(")") if pre_dot
    end

    def self.pp_int(q, term, pre_dot=false)
      return pp_int_datum(q, term.datum, pre_dot) if term.type == Term2::TermType::DATUM

      if term.type == Term2::TermType::VAR
        q.text("var_")
        return pp_int_datum(q, term.args[0].datum, false)
      end

      name = term.type.to_s.downcase
      args = term.args.dup
      optargs = term.optargs.dup

      if first_arg = args.shift
        pp_int(q, first_arg, true)
        q.text(".")
      end
      q.text(name)

      func = args[-1] && args[-1].type == Term2::TermType::FUNC && args.pop
      if args != [] || optargs != []
        q.group(0, "(", ")") {
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
          }
          q.breakable('')
        }
      end

      if func
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
    end

    def self.pp term
      q = PrettyPrint.new
      pp_int(q, term)
      q.flush
      q.output
    end
  end
end
