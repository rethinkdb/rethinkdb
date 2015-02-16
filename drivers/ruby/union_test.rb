load 'quickstart3.rb'

r.table_create('test').run rescue nil
$t = r.table('test')

$streams = [
  $t,
  $t.between(0, 100, index:'id'),
  #r([$t.get(0)]),
  #r([1, 2, 3, 4])
]

$infstreams = [
  $t.changes(),
  $t.between(10, 100, index: 'id').changes(),
  $t.get(0).changes()
]

$empty_streams = [
  $t.filter{false},
  r([$t.get('a')]).filter{|x| x},
  r([])
]

$empty_infstreams = [
  $t.filter{false}.changes(),
  $t.changes().filter{false},
  $t.get(0).changes().skip(1)
]

def flat_union(streams)
  r.union(*streams)
end

def nested_union(streams)
  streams.reduce{|a,b| r.union(a, b)}
end

$t.delete.run
$t.insert((0...100).map{|i| {id: i}}).run

$eager_pop = $streams
$eager_union_1 = flat_union($eager_pop)
$eager_union_2 = nested_union($eager_pop)

def assert
  res = yield
  raise "Assertion failed" if not res
  res
end

def cmp(a, b)
  if a.class != b.class
    return a.class.to_s <=> b.class.to_s
  elsif a.class == Hash
    return a.to_a <=> b.to_a
  else
    return a <=> b
  end
end

def mangle stream
  stream.to_a.sort{|x,y| cmp(x, y)}
end

(0...100).each {
  timeout(60) {
    $eager_canon = []
    $eager_pop.each{|pop|
      $eager_canon += pop.run.to_a
    }
    $eager_canon = mangle($eager_canon)
    $eager_res1 = mangle($eager_union_1.run)
    assert{$eager_res1 == $eager_canon}
    #$eager_res2 = mangle($eager_union_2.run)
    #assert{$eager_res2 == $eager_canon}
  }
}
