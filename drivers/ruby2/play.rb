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

def nativize_datum d
  dt = Datum::DatumType
  case d.type
  when dt::R_NUM then d.r_num
  when dt::R_STR then d.r_str
  when dt::R_BOOL then d.r_bool
  when dt::R_NULL then nil
  when dt::R_ARRAY then d.r_array.map{|d2| nativize_datum d2}
  when dt::R_OBJECT then Hash[d.r_object.map{|x| [x.key, nativize_datum(x.val)]}]
  else raise RuntimeError, "Unimplemented."
  end
end

def nativize_response r
  rt = Response2::ResponseType
  case r.type
  when rt::SUCCESS_ATOM then nativize_datum(r.response[0])
  when rt::SUCCESS_SEQUENCE then r.response.map{|d| nativize_datum(d)}
  when rt::RUNTIME_ERROR then
    raise RuntimeError, "#{r.response[0].r_str}
Backtrace: #{r.backtrace.map{|x| x.pos || x.opt}.inspect}"
  when rt::COMPILE_ERROR then # TODO: remove?
    raise RuntimeError, "#{r.response[0].r_str}
Backtrace: #{r.backtrace.map{|x| x.pos || x.opt}.inspect}"
  else r
  end
end

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
    q.token = @@token_cnt += 1
    return q
  end
  def run
    q = to_query
    payload = q.serialize_to_string
    packet = [payload.length].pack('L<') + payload
    @@socket.send(packet, 0)
    rlen = @@socket.read(4).unpack('L<')[0]
    r = @@socket.read(rlen)
    return nativize_response(Response2.new.parse_from_string(r))
  end

  def opt(key, val)
    ap = Term2::AssocPair.new
    ap.key = key
    ap.val = r(val).to_term
    self.to_term.optargs << ap
    return self
  end
end

def datum x
  dt = Datum::DatumType
  d = Datum.new
  return x if x.class == Datum
  case x.class.hash
  when Fixnum.hash then d.type = dt::R_NUM; d.r_num = x
  when Float.hash then d.type = dt::R_NUM; d.r_num = x
  when String.hash then d.type = dt::R_STR; d.r_str = x
  when TrueClass.hash then d.type = dt::R_BOOL; d.r_bool = x
  when FalseClass.hash then d.type = dt::R_BOOL; d.r_bool = x
  when NilClass.hash then d.type = dt::R_NULL
  when Array.hash then d.type = dt::R_ARRAY; d.r_array = x.map {|x| datum x}
  when Hash.hash then d.type = dt::R_OBJECT; d.r_object = x.map { |k,v|
      ap = Datum::AssocPair.new; ap.key = k; ap.val = datum v; ap }
  else raise RuntimeError, "Unimplemented."
  end
  return d
end

# Hash.hash

def r(a='8d19cb41-ddb8-44ab-92f3-f48ca607ab7b')
  return RQL.new if a == '8d19cb41-ddb8-44ab-92f3-f48ca607ab7b'
  return a if a.class == RQL
  d = datum(a)
  t = Term2.new
  t.type = Term2::TermType::DATUM
  t.datum = d
  return RQL.new t
end

$tests = 0
def assert_equal(a, b)
  raise RuntimeError, "#{a.inspect} != #{b.inspect}" if a != b
  $tests += 1
end

def assert_raise
  begin
    res = yield
  rescue Exception => e
  end
  raise RuntimeError, "Got #{res.inspect} instead of raise." if res
end

def rae(a, b)
  assert_equal(a.run, b)
end

rae(r(1), 1)
rae(r(2.0), 2.0)
rae(r("3"), "3")
rae(r(true), true)
rae(r(false), false)
rae(r(nil), nil)
rae(r([1, 2.0, "3", true, false, nil]), [1, 2.0, "3", true, false, nil])
rae(r({"abc" => 2.0, "def" => nil}), {"abc" => 2.0, "def" => nil})
rae(r.eq(1, 1), true)
rae(r.eq(1, 2), false)
rae(r.add(1, 2, 3, -1, 0.2), 5.2)
rae(r.sub(1, 2, 3), -4.0)
rae(r.mul(2, 3, 2, 0.5), 6.0)
rae(r.div(1, 2, 3), 1.0/6.0)
rae(r.div(1, 6), r.div(1, 2, 3).run)
rae(r.div(0.2), 0.2)
assert_raise{r.div(1, 0).run}
assert_raise{r.db('test').table('test2').run}

tbl = tst = r.db('test').table('test')
rae(tst.map(r.func([1], r.var(1))), tst.run)
rae(tst.map(r.func([1], 2)), tst.run.map{|x| 2})
rae(r([1,2,3]).map(r.func([1], r.mul(r.var(1), r.var(1), 2))), [2.0, 8.0, 18.0])
assert_raise{r([1,2,3]).map(r.func([1], r.mul(r.var(1), r.var(2), 2))).run}
assert_raise{r([1,2,3]).map(r.func([1], 2)).map(2).run}
rae(r([1,2,3]).map(r.func([1], r.var(1).mul(2))).map(r.func([1], r.var(1).add(1))),
    [3.0, 5.0, 7.0])

rae(r([1,2,3]).map(r.func([1], r.implicit_var.mul(r.var(1)))), [1.0, 4.0, 9.0])
assert_raise{r([1,2,3]).map(r.func([1], r([2,4]).map(r.func([2], r.implicit_var.mul(r.var(1)))))).run}
assert_raise{r.implicit_var.run}

rae(tbl.get(0), {"id"=>0.0})
rae(tbl.get(1), {"id"=>1.0})
rae(tbl.get(-1), nil)

rae(r(5).mod(3), 2)
assert_raise{r(5.2).mod(3).run}
assert_raise{r(5).mod(0).run}

print "test.test: #{r.db('test').table('test').run.inspect}\n"
print "Ran #{$tests} tests!\n"
