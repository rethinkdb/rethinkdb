module RethinkDB
  class RqlError < RuntimeError; end
  class RqlRuntimeError < RqlError; end
  class RqlConnectionError < RqlRuntimeError; end
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
