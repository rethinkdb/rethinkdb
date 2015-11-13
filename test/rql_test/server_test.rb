#!/usr/bin/env ruby

begin
  require 'minitest/autorun'
  UNIT_TEST_CLASS = MiniTest::Test
rescue
  require 'test/unit'
  UNIT_TEST_CLASS = Test::Unit::TestCase
end
require 'pp'

# -- import the rethinkdb driver

require_relative './importRethinkDB.rb'

# --

$port = (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i
ARGV.clear
$c = r.connect(port: $port).repl

$run_exc = RethinkDB::ReqlRuntimeError
$comp_exc = RethinkDB::ReqlCompileError

$s1 = r((0...10).map{|i| {a: i, b: i%2, c: i%3, d: i%5}})
$tbl1 = r.db('test').table('1')
$s2 = r((10...20).map{|i| {a: i, b: i%2, c: i%3, d: {e: i%5}}})
$tbl2 = r.db('test').table('2')
$s12 = r($s1 + $s2)
$tbl12 = r.db('test').table('12')
s_empty = r([])
$tbl_empty = r.db('test').table('empty')

def synonyms(seq)
  [seq,
   seq.map{|x| x},
   seq.filter{true},
   seq.limit(20),
   seq.orderby('a'),
   seq.orderby('a').limit(5).union(seq.orderby('a').skip(5))]
end
seq1s = synonyms($s1) + synonyms($tbl1)
seq2s = synonyms($s2) + synonyms($tbl2)
seq12s = synonyms($s12) + synonyms($tbl12)
empties = synonyms(s_empty) + synonyms($tbl_empty)
$sources = empties.map{|e|
  seqs = seq1s.map{|a|
    seq2s.map{|b|
      [a.union(b), a.union(b).union(e), a.union(e).union(b), e.union(a).union(b)]
    }
  } + seq12s.map{|ab| [ab, ab.union(e), e.union(ab)]}
}.flatten

$batch_confs = [{}, {max_batch_rows: 3}]

$slow = nil

$gmrdata = [{"a"=>0, "arr"=>[0, 0], "id"=>0}, {"a"=>1, "arr"=>[1, 1], "b"=>1, "id"=>1}, {"a"=>2, "arr"=>[0, 2], "b"=>2, "id"=>2}, {"a"=>0, "arr"=>[1, 3], "id"=>3}, {"a"=>1, "arr"=>[0, 4], "id"=>4}, {"a"=>2, "arr"=>[1, 0], "b"=>2, "id"=>5}, {"a"=>0, "arr"=>[0, 1], "id"=>6}, {"a"=>1, "arr"=>[1, 2], "b"=>1, "id"=>7}, {"a"=>2, "arr"=>[0, 3], "b"=>2, "id"=>8}, {"a"=>0, "arr"=>[1, 4], "id"=>9}]
$tbl = r.db('test').table('gmrdata')

class ClientTest < UNIT_TEST_CLASS

  def setup
    r.db_create('test').run rescue nil
    r.db('test').table_create('test').run rescue nil
    r.db('test').table_create('1').run rescue nil
    r.db('test').table_create('2').run rescue nil
    r.db('test').table_create('12').run rescue nil
    r.db('test').table_create('empty').run rescue nil
    r.db('test').table_create('gmrdata').run rescue nil
    r.db('test').table_list.foreach{|x| r.db('test').table(x).delete}.run
    $tbl1.insert($s1).run
    $tbl2.insert($s2).run
    $tbl12.insert($s12).run
    $tbl.insert($gmrdata).run
    $tbl.index_create('a').run rescue nil
    $tbl.index_create('b').run rescue nil
    $tbl.index_create('arr').run rescue nil
    $tbl.index_wait.run
  end

  def eq(query, res, &b)
    $batch_confs.map {|bc|
      qres = query.run(bc)
      qres = qres.to_a if qres.is_a?(RethinkDB::Cursor)
      qres = b.call(qres) if b
      assert_equal(res, qres, "
...............................................................................
Query: #{PP.pp(query, "")}\nBatch Conf: #{bc}
..............................................................................")
    }
  end

  def percent_iter(arr, rat=100)
    i=0
    arr.each {|el|
      yield(el)
      if (i+=1)%(arr.size()/rat) == 0
        puts("#{i.to_f/arr.size()}")
      end
    }
  end

  def iter_sources
    canon = yield($s12).run
    PP.pp yield($s12)
    PP.pp canon
    percent_iter($sources) {|source|
      eq(yield(source), canon)
    }
  end

  def test_malformed_queries
    class << $c
      def dispatch(msg, token)
        payload = RethinkDB::Shim.dump_json(msg).force_encoding('BINARY')
        payload = $dispatch_hook.call(payload) if $dispatch_hook
        prefix = [token, payload.bytesize].pack('Q<L<')
        send(prefix + payload)
        return token
      end
    end

    begin
      assert_raises(RethinkDB::ReqlDriverError) {
        $dispatch_hook = lambda {|x| x.gsub('[', '{')}
        eq(r(1), 1)
      }
      assert_raises(RethinkDB::ReqlDriverError) {
        $dispatch_hook = lambda {|x| x.gsub('1', '\u0000')}
        eq(r(1), 1)
      }
    ensure
      $dispatch_hook = nil
    end
    $c.register_query(1337, {})
    assert_equal({ "t"=>16, "b"=>[], "r"=>["Server could not parse query: Expected 1 to 3 elements in the top-level query, but found 4."] },
                 $c.wait($c.dispatch([1, 1337, 1, {}], 1337), nil))

    $c.register_query(-1, {})
    assert_equal({ "t"=>16, "b"=>[], "r"=>["Server could not parse query: Expected a query type as a number, but found STRING."] },
                 $c.wait($c.dispatch(["a", 1337, {}], -1), nil))

    $c.register_query(16, {})
    assert_equal({ "t"=>16, "b"=>[], "r"=>["Server could not parse query: Expected global optargs as an object, but found NUMBER."] },
                 $c.wait($c.dispatch([1, 1337, 1], 16), nil))

    $c.register_query(50, {})
    assert_equal({ "t"=>16, "b"=>[], "r"=>["Server could not parse query: Unrecognized QueryType: 5843."] },
                 $c.wait($c.dispatch([5843, 1, {}], 50), nil))

    $c.register_query(82, {})
    assert_equal({ "t"=>17, "b"=>[], "r"=>["Unrecognized TermType: 45784."] },
                 $c.wait($c.dispatch([1, [45784], {}], 82), nil))

    # Test some backtraces
    $c.register_query(82, {})
    assert_equal({ "t"=>17, "b"=>[1], "r"=>["Unrecognized TermType: 45784."] },
                 $c.wait($c.dispatch([1, [24, [1, [45784]]], {}], 82), nil))

    $c.register_query(82, {})
    assert_equal({ "t"=>17, "b"=>['data'], "r"=>["Unrecognized TermType: 45785."] },
                 $c.wait($c.dispatch([1, [24, [1, 1], {'data'=>[45785]}], {}], 82), nil))

    $c.register_query(82, {})
    assert_equal({ "t"=>17, "b"=>['data', 0], "r"=>["Unrecognized TermType: 45786."] },
                 $c.wait($c.dispatch([1, [24, [1, 1], {'data'=>[24, [[45786]]]}], {}], 82), nil))
  end

  def test_gmr_slow
    if $slow
      eq(r([]).group('a').count, {})
      eq($tbl_empty.group('a').count, {})
      iter_sources {|s|
        s.orderby('a').group('b', 'c').map{|x| x['a']}.reduce{|a,b| a+b}
      }
      iter_sources {|s| s.group('b', 'c').min('a')}
      iter_sources {|s| s.group('b', 'c').max('a')}
      iter_sources {|s| s.group('b', 'c').sum('a')}
      iter_sources {|s| s.group('b', 'c').avg('a')}
      iter_sources {|s| s.group('b', 'c').count}
      iter_sources {|s| s.group('b', 'c').count{|x| x['a'].eq(0)}}
      iter_sources {|s|
        s.group('b', 'c').map{|x| [x]}.reduce{|a,b| a+b}
      }
    end
  end

  def test_group_coercion
    eq($tbl.orderby(index:'id'), $gmrdata)
    gmrdata_grouped = {
      0 => [{"a"=>0, "arr"=>[0, 0], "id"=>0},
            {"a"=>0, "arr"=>[1, 3], "id"=>3},
            {"a"=>0, "arr"=>[0, 1], "id"=>6},
            {"a"=>0, "arr"=>[1, 4], "id"=>9}],
      1 => [{"a"=>1, "arr"=>[1, 1], "b"=>1, "id"=>1},
            {"a"=>1, "arr"=>[0, 4], "id"=>4},
            {"a"=>1, "arr"=>[1, 2], "b"=>1, "id"=>7}],
      2 => [{"a"=>2, "arr"=>[0, 2], "b"=>2, "id"=>2},
            {"a"=>2, "arr"=>[1, 0], "b"=>2, "id"=>5},
            {"a"=>2, "arr"=>[0, 3], "b"=>2, "id"=>8}]}
    [$tbl, $tbl.orderby(index:'arr'), $tbl.orderby('a'), r($gmrdata)].each {|seq|
      eq(seq.group('a'), gmrdata_grouped) {|x|
        Hash[x.map{|k,v| [k, v.sort{|a,b| a['id'] <=> b['id']}]}]
      }
      eq(seq.group('a').typeof, "GROUPED_STREAM")
      eq(seq.group('a').do{|x| x}, gmrdata_grouped) {|x|
        Hash[x.map{|k,v| [k, v.sort{|a,b| a['id'] <=> b['id']}]}]
      }
      eq(seq.group('a').do{|x| x}.typeof, "GROUPED_DATA")
      eq(seq.group('a').orderby('id'), gmrdata_grouped)
      eq(seq.group('a').orderby('id').typeof, "GROUPED_DATA")
    }
    eq($tbl.orderby('id').group('a'), gmrdata_grouped)
    eq($tbl.orderby(index:'id').group('a'), gmrdata_grouped)
  end

  def test_group_ops
    [$tbl, $tbl.orderby(index:'arr'), $tbl.orderby('a'), r($gmrdata)].each {|seq|
      eq(seq.group('a').sum('a'), {0 => 0, 1 => 3, 2 => 6})
      eq(seq.group('a').sum('b'), {1 => 2, 2 => 6})
      eq(seq.group('a').sum('c'), {})
      eq(seq.group('a').sum('id'), {0=>18, 1=>12, 2=>15})

      eq(seq.group('a')['a'].sum, {0 => 0, 1 => 3, 2 => 6})
      eq(seq.group('a')['b'].sum, {1 => 2, 2 => 6})
      eq(seq.group('a')['c'].sum, {})
      eq(seq.group('a')['id'].sum, {0=>18, 1=>12, 2=>15})

      eq(seq.group('a').avg('a'), {0 => 0, 1 => 1, 2 => 2})
      eq(seq.group('a').avg('b'), {1 => 1, 2 => 2})
      eq(seq.group('a').avg('c'), {})
      eq(seq.group('a').avg('id'), {0=>4.5, 1=>4, 2=>5})

      eq(seq.group('a')['a'].avg, {0 => 0, 1 => 1, 2 => 2})
      eq(seq.group('a')['b'].avg, {1 => 1, 2 => 2})
      eq(seq.group('a')['c'].avg, {})
      eq(seq.group('a')['id'].avg, {0=>4.5, 1=>4, 2=>5})

      eq(seq.group('a').min('a'), {0 => 0, 1 => 1, 2 => 2}) {|x|
        Hash[x.map{|k,v| [k, v['a']]}]
      }
      eq(seq.group('a').min(), {0 => 0, 1 => 1, 2 => 2}) {|x|
        Hash[x.map{|k,v| [k, v['a']]}]
      }
      eq(seq.group('a').min('b'), {1 => 1, 2 => 2}) {|x|
        Hash[x.map{|k,v| [k, v['a']]}]
      }
      eq(seq.group('a').min('c'), {})
      eq(seq.group('a').min('arr'),
         { 0=>{"a"=>0, "arr"=>[0, 0], "id"=>0},
           1=>{"a"=>1, "arr"=>[0, 4], "id"=>4},
           2=>{"a"=>2, "arr"=>[0, 2], "b"=>2, "id"=>2}})
      eq(seq.group('a').min('id'),
         { 0=>{"a"=>0, "arr"=>[0, 0], "id"=>0},
           1=>{"a"=>1, "arr"=>[1, 1], "b"=>1, "id"=>1},
           2=>{"a"=>2, "arr"=>[0, 2], "b"=>2, "id"=>2}})

      eq(seq.group('a').max('a'), {0 => 0, 1 => 1, 2 => 2}) {|x|
        Hash[x.map{|k,v| [k, v['a']]}]
      }
      eq(seq.group('a').max(), {0 => 0, 1 => 1, 2 => 2}) {|x|
        Hash[x.map{|k,v| [k, v['a']]}]
      }
      eq(seq.group('a').max('b'), {1 => 1, 2 => 2}) {|x|
        Hash[x.map{|k,v| [k, v['a']]}]
      }
      eq(seq.group('a').max('c'), {})
      eq(seq.group('a').max('arr'),
         { 0=>{"a"=>0, "arr"=>[1, 4], "id"=>9},
           1=>{"a"=>1, "arr"=>[1, 2], "b"=>1, "id"=>7},
           2=>{"a"=>2, "arr"=>[1, 0], "b"=>2, "id"=>5}})
      eq(seq.group('a').max('id'),
         { 0=>{"a"=>0, "arr"=>[1, 4], "id"=>9},
           1=>{"a"=>1, "arr"=>[1, 2], "b"=>1, "id"=>7},
           2=>{"a"=>2, "arr"=>[0, 3], "b"=>2, "id"=>8}})
    }
  end

end


