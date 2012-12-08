$LOAD_PATH.unshift('./lib')
require 'rubygems'
require 'pp'
require 'ql2.pb.rb'
require 'socket'

def to_termtype sym
  eval "Term2::TermType::#{sym.to_s.upcase}"
end

def check pred
  raise RuntimeError, "CHECK FAILED" if not pred
end

$socket = TCPSocket.open('localhost', 60616)
$socket.send([0xaf61ba35].pack('L<'), 0)

class RQL
  attr_accessor :is_r
  def initialize(term=nil)
    @term = term
    @is_r = !term
  end
  def method_missing(m, *a, &b)
    t = Term2.new
    t.type = to_termtype(m)
    t.args << self.to_term if !@is_r
    a.each {|arg| t.args << r(arg).to_term}
    return RQL.new t
  end
  def to_term
    check @term
    @term
  end

  @@token_cnt = 0
  @@socket = $socket

  def to_query
    q = Query2.new
    q.type = Query2::QueryType::START
    q.query = self.to_term
    q.token = @@token_cnt + 1
    return q
  end
  def run
    q = to_query
    payload = q.serialize_to_string
    packet = [payload.length].pack('L<') + payload
    @@socket.send(packet, 0)
    rlen = @@socket.read(4).unpack('L<')[0]
    r = @@socket.read(rlen)
    return Response2.new.parse_from_string(r)
  end
end

def datum x
  dt = Datum::DatumType
  d = Datum.new
  return x if x.class == Datum
  case x.class.hash
  when Fixnum.hash then d.type = dt::R_NUM; d.r_num = x
  else raise RuntimeError, "Unimplemented."
  end
  return d
end

def r(a='8d19cb41-ddb8-44ab-92f3-f48ca607ab7b')
  return RQL.new if a == '8d19cb41-ddb8-44ab-92f3-f48ca607ab7b'
  return a if a.class == RQL
  d = datum(a)
  t = Term2.new
  t.type = Term2::TermType::DATUM
  t.datum = d
  return RQL.new t
end

r.add(1, 2).run
r(1).run
