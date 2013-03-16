# Copyright 2010-2012 RethinkDB, all rights reserved.
require 'rubygems'
require 'ql2.pb.rb'

if 2**63 != (2**62)*2
  puts "WARNING: Ruby believes 2**63 = #{2**63} rather than #{(2**62)*2}!"
  puts "Consider upgrading your verison of Ruby."
  puts "Hot-patching ruby_protobuf to compensate..."
  rethinkdb_verbose, $VERBOSE = $VERBOSE, nil
  module Protobuf
    module Field
      class VarintField < BaseField
        INT64_MAX = (2**62)*2 - 1
        INT64_MIN = (2**62)*-2
        UNT64_MAX = (2**62)*4 - 1
      end
    end
  end
  $VERBOSE = rethinkdb_verbose
end

require 'socket'
require 'pp'

load 'exc.rb'
load 'net.rb'
load 'shim.rb'
load 'func.rb'
load 'rpp.rb'

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
