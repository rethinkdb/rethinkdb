require_relative './importRethinkDB.rb'

$port ||= (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i
ARGV.clear
$c = r.connect(port: $port).repl

r.table_create('test').run rescue nil
$t = r.table('test')

$streams = [
  $t,
  $t.between(10, 100, index:'id'),
  r([$t.get(0)]),
  r([1, 2, 3, 4])
]

$infstreams = [
  $t.changes(),
  $t.between(10, 100, index: 'id').changes(include_initial: false),
  $t.get(0).changes(include_initial: true).skip(1)
]

# TODO: uncomment these once we increase the stack size.  (Issue #4348.)
$empty_streams = [
  $t.filter{false},
  #r([$t.get('a')]).filter{|x| x},
  #r([])
]

# TODO: uncomment these once we increase the stack size.  (Issue #4348.)
$empty_infstreams = [
  $t.filter{false}.changes(),
  # $t.changes().filter{false},
  # $t.get('a').changes().skip(1)
]

def flat_union(streams)
  r.union(*streams)
end

def nested_union(streams)
  streams.reduce{|a,b| r.union(a, b)}
end

$t.delete.run
$t.insert((0...100).map{|i| {id: i}}).run

$eager_pop = $streams*2 + $empty_streams*2 + $streams
$eager_union_1 = flat_union($eager_pop)
$eager_union_2 = nested_union($eager_pop)

def assert
  res = yield
  raise "Assertion failed" if not res
  res
end

def dehash x
  x.is_a?(Hash) ? x.map{|k,v| [k, dehash(v)]} : x
end

def cmp(a, b)
  if a.class != b.class
    return a.class.to_s <=> b.class.to_s
  else
    return dehash(a) <=> dehash(b)
  end
end

def mangle stream
  stream.to_a.sort{|x,y| cmp(x, y)}
end

puts "Testing eager unions."
timeout(10) {
  $eager_canon = []
  $eager_pop.each{|pop|
    $eager_canon += pop.run.to_a
  }
  $eager_canon = mangle($eager_canon)
  $eager_res1 = mangle($eager_union_1.run)
  assert{$eager_res1 == $eager_canon}
  $eager_res2 = mangle($eager_union_2.run)
  assert{$eager_res2 == $eager_canon}
}

puts "Constructing lazy unions."
timeout(20) {
  $lazy_pop = $empty_infstreams + $eager_pop + $empty_infstreams
  $change_pop = $infstreams + $lazy_pop + $infstreams
  $lp1 = flat_union($lazy_pop).run.each
  $lp2 = nested_union($lazy_pop).run.each
  $cp1 = flat_union($change_pop).run.each
  $cp2 = nested_union($change_pop).run.each
  # TODO: .union.changes
}

puts "Testing lazy unions."
timeout(10) {
  $lp1res = []
  $lp2res = []
  $cp1res = []
  $cp2res = []
  $eager_canon.each_index {|i|
    $lp1res << $lp1.next
    $lp2res << $lp2.next
    $cp1res << $cp1.next
    $cp2res << $cp2.next
  }
  $lp1res = mangle($lp1res)
  $lp2res = mangle($lp2res)
  $cp1res = mangle($cp1res)
  $cp2res = mangle($cp2res)
  assert{$lp1res == $eager_canon}
  assert{$lp2res == $eager_canon}
  assert{$cp1res == $eager_canon}
  assert{$cp2res == $eager_canon}
}

puts "Testing updates on open changefeeds."
timeout(10) {
  $t.get(0).update({a: 1}).run
  $t.get(5).update({a: 1}).run
  $t.get(50).update({a: 1}).run
  $expected_changes =
    [{"new_val"=>{"a"=>1, "id"=>0}, "old_val"=>{"id"=>0}}]*4 +
    [{"new_val"=>{"a"=>1, "id"=>5}, "old_val"=>{"id"=>5}}]*2 +
    [{"new_val"=>{"a"=>1, "id"=>50}, "old_val"=>{"id"=>50}}]*4
  $expected_changes = mangle($expected_changes)
  $res1 = []
  $res2 = []
  $expected_changes.each_index {|i|
    $res1 << $cp1.next
    $res2 << $cp2.next
  }
  $res1 = mangle($res1)
  $res2 = mangle($res2)
  assert{$res1 == $expected_changes}
  assert{$res2 == $expected_changes}
}

$timed_out = false
begin
  timeout(2) {
    $lp1.next
  }
rescue Timeout::Error => e
  $timed_out = true
end
assert{$timed_out}

$timed_out = false
begin
  timeout(2) {
    $lp2.next
  }
rescue Timeout::Error => e
  $timed_out = true
end
assert{$timed_out}

r.table_drop('test')
$errored = false
begin
  timeout(2) {
    $lp1.next
  }
rescue RethinkDB::ReqlRuntimeError => e
  $errored = true
end
assert{$errored}
