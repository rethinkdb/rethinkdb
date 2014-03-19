require 'json'
require 'time'

# RSI: move `require`s into module

class TIME
  def self.__rdb_new_json_create(o)
    t = Time.at(o['epoch_time'])
    tz = o['timezone']
    (tz && tz != "" && tz != "Z") ? t.getlocal(tz) : t.utc
  end
  if !methods.include?(:json_create)
    singleton_class.send(:alias_method, :json_create, :__rdb_new_json_create)
  end
end

class Time
  def __rdb_new_to_json(*a, &b)
    epoch_time = self.to_f
    offset = self.utc_offset
    raw_offset = offset.abs
    raw_hours = raw_offset / 3600
    raw_minutes = (raw_offset / 60) - (raw_hours * 60)
    timezone = (offset < 0 ? "-" : "+") + sprintf("%02d:%02d", raw_hours, raw_minutes);
    {
      '$reql_type$' => 'TIME',
      'epoch_time'  => epoch_time,
      'timezone'    => timezone
    }.to_json(*a, &b)
  end
  alias_method :__rdb_old_to_json, :__rdb_new_to_json
end

module RethinkDB
  module Shim
    def self.load_json(*a, &b)
      old_create_id = JSON.create_id
      begin
        ::TIME.singleton_class.send(:alias_method,
                                    :__rdb_old_json_create, :json_create)
        ::TIME.singleton_class.send(:alias_method,
                                    :json_create, :__rdb_new_json_create)
        JSON.create_id = '$reql_type$'
        JSON.load(*a, &b)
      ensure
        JSON.create_id = old_create_id
        ::TIME.singleton_class.send(:alias_method,
                                    :json_create, :__rdb_old_json_create)
      end
    end

    def self.dump_json(*a, &b)
      begin
        ::Time.class_eval {
          alias_method :__rdb_old_to_json, :to_json
          alias_method :to_json, :__rdb_new_to_json
        }
        JSON.dump(*a, &b)
      ensure
        ::Time.class_eval {
          alias_method :to_json, :__rdb_old_to_json
        }
      end
    end

    def self.response_to_native(r, orig_term, opts)
      rt = Response::ResponseType
      # if r.backtrace
      #   bt = r.backtrace.frames.map {|x|
      #     x.type == Frame::FrameType::POS ? x.pos : x.opt
      #   }
      # else
      #   bt = []
      # end

      # RSI: pseudotypes

      begin
        case r['t']
        when rt::SUCCESS_ATOM then r['r'][0]
        when rt::SUCCESS_PARTIAL then r['r']
        when rt::SUCCESS_SEQUENCE then r['r']
        when rt::RUNTIME_ERROR then
          raise RqlRuntimeError, r['r'][0]
        when rt::COMPILE_ERROR then # TODO: remove?
          raise RqlCompileError, r['r'][0]
        when rt::CLIENT_ERROR then
          raise RqlDriverError, r['r'][0]
        else raise RqlRuntimeError, "Unexpected response: #{r.inspect}"
        end
      rescue RqlError => e
        # term = orig_term.deep_dup
        # term.bt_tag(bt)
        # raise e.class, "#{e.message}\nBacktrace:\n#{RPP.pp(term)}"
        raise e.class, "#{e.message}\nRSI"
      end
    end
  end

  class RQL
    def to_json(*a, &b)
      @body.to_json
    end
    def to_pb; @body; end

    def self.safe_to_s(x)
      case x
      when String then x
      when Symbol then x.to_s
      else raise RqlRuntimeError, "RSI"
      end
    end

    def self.fast_expr(x)
      case x
      when RQL then x
      when Array then RQL.new({ t: Term::TermType::MAKE_ARRAY,
                                a: x.map{|y| fast_expr(y)} })
      when Hash then RQL.new({ t: Term::TermType::MAKE_OBJ,
                               o: x.map {|k,v|
                                 { k: safe_to_s(k),
                                   v: fast_expr(v) }
                               } })
      else RQL.new({t: Term::TermType::DATUM, d: x})
      end
    end

    def expr(x)
      unbound_if @body
      RQL.fast_expr(x)
    end
    def coerce(other)
      [RQL.new.expr(other), self]
    end
  end
end
