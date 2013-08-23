#!/usr/bin/ruby
require 'json'
require 'pp'
$x = JSON.parse(File.read(ARGV[0]))

class Object
  def method_missing(m, *a)
    raise NoMethodError,m.to_s if m == :to_i || m == :to_ary
    nil
  end
end

class Array
  def method_missing(m, *a)
    raise NoMethodError,m.to_s if m == :to_i || m == :to_ary
    arr = self.map{|x| x.send(m, *a)}.delete_if{|x| x == nil}
    return arr
    return (arr == []) ? nil : arr
  end
end
class Hash
  def method_missing(m, *a)
    raise NoMethodError,m.to_s if m == :to_i || m == :to_ary
    if m.to_s == "_"
      self.keys.map{|x| self[x]}
    else
      self[m.to_s]
    end
  end
end

PP.pp $x.instance_eval(ARGV[1]).flatten.map{|s|
  begin
    n = s.to_i
  rescue NoMethodError => e
    n = 0
  end
  (n == 0 && s != "0") ? s : n
}

# PP.pp $x.instance_eval(ARGV[1])
