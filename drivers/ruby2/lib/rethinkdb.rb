# Copyright 2010-2012 RethinkDB, all rights reserved.
require 'rubygems'
require 'ql2.pb.rb'
require 'socket'
require 'pp'

load 'exc.rb'
load 'net.rb'
load 'shim.rb'

module RethinkDB
  def self.r_int(*args)
    args == [] ? RQL.new : r.expr(*args)
  end
  def r(*args); RethinkDB.r_int(*args); end

  module Shortcuts
    def r(*args); RethinkDB.r_int(*args); end
  end

  module Utils
    def get_mname
      caller[0]=~/`(.*?)'/
      $1
    end
    def unbound_if x
      raise NoMethodError, "undefined method `#{get_mname}'" if x
    end
  end

  class RQL
    include Utils
  end
end
