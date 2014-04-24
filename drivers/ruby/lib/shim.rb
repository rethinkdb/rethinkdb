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

class GROUPED_DATA
  def self.__rdb_new_json_create(o)
    Hash[o['data']]
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
    { '$reql_type$' => 'TIME',
      'epoch_time'  => epoch_time,
      'timezone'    => timezone }.to_json(*a, &b)
  end
end

module RethinkDB
  require 'json'
  require 'time'
  module Shim
    def self.load_json(target, opts=nil)
      old_create_id = JSON.create_id
      begin
        ::TIME.singleton_class.send(:alias_method,
                                    :__rdb_old_json_create, :json_create)
        ::TIME.singleton_class.send(:alias_method,
                                    :json_create, :__rdb_new_json_create)
        if opts && opts[:time_format] == 'raw'
          ::TIME.singleton_class.send(:remove_method, :json_create)
        end
        ::GROUPED_DATA.singleton_class.send(:alias_method,
                                            :__rdb_old_json_create, :json_create)
        ::GROUPED_DATA.singleton_class.send(:alias_method,
                                            :json_create, :__rdb_new_json_create)
        if opts && opts[:group_format] == 'raw'
          ::GROUPED_DATA.singleton_class.send(:remove_method, :json_create)
        end
        JSON.create_id = '$reql_type$'
        JSON.load(target)
      ensure
        JSON.create_id = old_create_id
        ::TIME.singleton_class.send(:alias_method,
                                    :json_create, :__rdb_old_json_create)
        ::GROUPED_DATA.singleton_class.send(:alias_method,
                                            :json_create, :__rdb_old_json_create)
      end
    end

    def self.dump_json(*a, &b)
      begin
        ::Time.class_eval {
          alias_method :__rdb_old_to_json, :to_json if methods.include?(:to_json)
          alias_method :to_json, :__rdb_new_to_json
        }
        JSON.dump(*a, &b)
      ensure
        ::Time.class_eval {
          if methods.include?(:__rdb_old_to_json)
            alias_method :to_json, :__rdb_old_to_json
          else
            remove_method :to_json
          end
        }
      end
    end

    def self.response_to_native(r, orig_term, opts)
      rt = Response::ResponseType
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
        raise e.class, "#{e.message}\nBacktrace:\n#{RPP.pp(orig_term, r['b'])}"
      end
    end
  end

  class RQL
    def to_json(*a, &b)
      @body.to_json(*a, &b)
    end
    def to_pb; @body; end

    def self.safe_to_s(x)
      case x
      when String then x
      when Symbol then x.to_s
      else raise RqlDriverError, 'Object keys must be strings or symbols.  '+
          "(Got object `#{x.inspect}` of class `#{x.class}`.)"
      end
    end

    def self.fast_expr(x, depth=0)
      raise RqlDriverError, "Maximum expression depth of 20 exceeded." if depth > 20
      case x
      when RQL then x
      when Array then RQL.new([Term::TermType::MAKE_ARRAY,
                               x.map{|y| fast_expr(y, depth+1)}])
      when Hash then RQL.new(Hash[x.map{|k,v| [safe_to_s(k), fast_expr(v, depth+1)]}])
      when Proc then RQL.new.new_func(&x)
      else RQL.new(x)
      end
    end

    def expr(x)
      unbound_if(@body != RQL)
      RQL.fast_expr(x)
    end
    def coerce(other)
      [RQL.new.expr(other), self]
    end
  end
end
