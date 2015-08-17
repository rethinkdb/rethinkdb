# Copyright 2010-2014 RethinkDB, all rights reserved.

require_relative './ql2.pb.rb'
require_relative './exc.rb'
require_relative './net.rb'
require_relative './shim.rb'
require_relative './func.rb'
require_relative './rpp.rb'

module RethinkDB
  module Shortcuts
    def r(*args)
      args == [] ? RQL.new : RQL.new.expr(*args)
    end
  end

  module Utils
    def get_mname(i = 0)
      caller(i, 1)[0] =~ /`(.*?)'/; $1
    end
    def unbound_if (x, name = nil)
      if x
        name = get_mname(2) if not name
        raise NoMethodError, "undefined method `#{name}'"
      end
    end
  end

  class RQL
    include Utils

    attr_accessor :body, :bitop

    def initialize(body = RQL, bitop = nil)
      @body = body
      @bitop = bitop
    end

    def pp
      unbound_if(@body == RQL)
      RethinkDB::RPP.pp(@body)
    end
    def inspect
      (@body != RQL) ? pp : super
    end
  end
end
