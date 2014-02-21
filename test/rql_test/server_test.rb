#!/usr/local/bin/ruby
$LOAD_PATH.unshift '../../drivers/ruby/lib'
$LOAD_PATH.unshift '../../build/drivers/ruby/rdb_protocol'
require 'pp'
require 'rethinkdb'
include RethinkDB::Shortcuts

$port ||= ARGV[0].to_i
ARGV = []
$c = r.connect(port: $port).repl

$run_exc = RethinkDB::RqlRuntimeError
$comp_exc = RethinkDB::RqlCompileError

$s1 = r((0...10).map{|i| {a: i, b: i%2, c: i%3, d: i%5}})
$tbl1 = r.table('1')
$s2 = r((10...20).map{|i| {a: i, b: i%2, c: i%3, d: {e: i%5}}})
$tbl2 = r.table('2')
$s12 = r($s1 + $s2)
$tbl12 = r.table('12')
s_empty = r([])
$tbl_empty = r.table('empty')

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

$batch_confs = [{}, {batch_conf: {max_els: 3}}]

require 'test/unit'
class ClientTest < Test::Unit::TestCase
  def setup
    r.db_create('test').run rescue nil
    r.table_create('test').run rescue nil
    r.table_create('1').run rescue nil
    r.table_create('2').run rescue nil
    r.table_create('12').run rescue nil
    r.table_create('empty').run rescue nil
    r.table_list.foreach{|x| r.table(x).delete}.run
    $tbl1.insert($s1).run
    $tbl2.insert($s2).run
    $tbl12.insert($s12).run
  end

  def eq(query, res)
    $batch_confs.map {|bc|
      assert_equal(res, query.run(bc),
                   "Query: #{PP.pp(query, "")}\nBatch Conf: #{bc}\n")
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

  def test_gmr
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


