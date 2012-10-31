# Copyright 2010-2012 RethinkDB, all rights reserved.
$LOAD_PATH.unshift('./rethinkdb')
require 'test/unit'
require 'rethinkdb.rb'
extend RethinkDB::Shortcuts
$port_base = ARGV[0].to_i # 0 if none given
$c = RethinkDB::Connection.new('localhost', $port_base + 28015)
begin
  r.db('test').create_table('tbl').run
rescue
end
$rdb = r.db('test').table('tbl')
$rdb.delete.run
$rdb.insert((0...10).map{|i| {:id => i}}).run

class ClientBacktraceTest < Test::Unit::TestCase
  include RethinkDB::Shortcuts
  def check(query, str1, str2, debug=false)
    begin
      RethinkDB::B.force_raise(query)
    rescue Exception => e
      print e.message if debug
      lines = e.message.split("\n")
      expected = [str1.gsub(/_var_[0-9]+/,'VAR'), str2]
      got = [lines[-2].gsub(/_var_[0-9]+/,'VAR'), lines[-1]]
      assert_equal(expected, got)
      return
    end
    assert_equal(true, false)
  end

  def test_simple_args
    check(r.add(1, "a"),
          'Query: add(1, "a")',
          '              ^^^')
    check(r.add(1, r.add(2, r.add(3, 4), r.add("a", 1)), r.add(5, 6)),
          'Query: add(1, add(2, add(3, 4), add("a", 1)), add(5, 6))',
          '                                    ^^^')
    check(r.mul(1, r.div(2, r.add(3, 4)-r.expr("a")+1), r.add(5, 6)),
          'Query: multiply(1, divide(2, add(subtract(add(3, 4), "a"), 1)), add(5, 6))',
          '                                                     ^^^')
  end

  def test_get_pick_has
    check(r.add(r.expr({:a => 1})[:b], 1),
          'Query: add({:a=>1}[:b], 1)',
          '           ^^^^^^^^^^^')
    check(r.add(r.expr({:a => r.add(1, "a")})[:b], 1),
          'Query: add({:a=>add(1, "a")}[:b], 1)',
          '                       ^^^')
    check(r.expr(1)[:b],
          'Query: 1[:b]',
          '       ^')

    check(r.add(r.expr({:a => 1}).pickattrs(:b), 1),
          'Query: add({:a=>1}.pickattrs(:b), 1)',
          '           ^^^^^^^^^^^^^^^^^^^^^')
    check(r.add(r.expr({:a => 1}).pickattrs(:b, :c), 1),
          'Query: add({:a=>1}.pickattrs(:b, :c), 1)',
          '           ^^^^^^^^^^^^^^^^^^^^^^^^^')
    check(r.expr(1).pickattrs(:a),
          'Query: 1.pickattrs(:a)',
          '       ^')

    check(r.expr(1).hasattr(:b),
          'Query: 1.hasattr(:b)',
          '       ^')

    check(r.expr(1).without(:id),
          'Query: 1.without(:id)',
          '       ^')
  end

  def test_if
    check(r.if(1, 2, 3),
          'Query: if(1, 2, 3)',
          '          ^')
    check(r.if(r.if(true,1,false),2,3),
          'Query: if(if(true, 1, false), 2, 3)',
          '          ^^^^^^^^^^^^^^^^^^')
    check(r.if(true, r.add(1, "a"), r.add("b", 2)),
          'Query: if(true, add(1, "a"), add("b", 2))',
          '                       ^^^')
    check(r.if(false, r.add(1, "a"), r.add("b", 2)),
          'Query: if(false, add(1, "a"), add("b", 2))',
          '                                  ^^^')
  end

  def test_point
    check(r.add($rdb.get(0, :bah), 1),
          'Query: add(getbykey(["test", "tbl"], :bah, 0), 1)',
          '           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^')
    check($rdb.get(0).update{|row| {:id => 1}},
          'Query: pointupdate(["test", "tbl"], :id, 0, ["_var_1001", {:id=>1}])',
          '                                                          ^^^^^^^^')
    check($rdb.get(0).mutate{|row| {:id => 2}},
          'Query: pointmutate(["test", "tbl"], :id, 0, ["_var_1002", {:id=>2}])',
          '                                                          ^^^^^^^^')
    check($rdb.get(0, :bah),
          'Query: getbykey(["test", "tbl"], :bah, 0)',
          '       ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^')
    check($rdb.get(0, :bah).update{{}},
          'Query: pointupdate(["test", "tbl"], :bah, 0, ["_var_1003", {}])',
          '       ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^')
    check($rdb.get(0, :bah).mutate{{}},
          'Query: pointmutate(["test", "tbl"], :bah, 0, ["_var_1004", {}])',
          '       ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^')

    check($rdb.get(0).update{{:new => r.add(1,"a")}},
          'Query: pointupdate(["test", "tbl"], :id, 0, ["_var_1046", {:new=>add(1, "a")}])',
          '                                                                        ^^^')
    check($rdb.get(0).mutate{{:new => r.add(1,"a")}},
          'Query: pointmutate(["test", "tbl"], :id, 0, ["_var_1047", {:new=>add(1, "a")}])',
          '                                                                        ^^^')
  end

  def test_reduce
    check($rdb.reduce(0){|a,b| a+b},
          'Query: db("test").table("tbl").reduce(0, "_var_1014", "_var_1015", add(_var_1014, _var_1015))',
          '                                                                                  ^^^^^^^^^')
    check($rdb.map{|x| x[:id]}.reduce(r.add(1,"a")){|a,b| a+b},
          'Query: db("test").table("tbl").map("_var_1016", _var_1016[:id]).reduce(add(1, "a"), "_var_1017", "_var_1018", add(_var_1017, _var_1018))',
          '                                                                              ^^^')
    check($rdb.map{|x| x[:id]}.reduce($rdb){|a,b| a+b},
          'Query: db("test").table("tbl").map("_var_1019", _var_1019[:id]).reduce(db("test").table("tbl"), "_var_1020", "_var_1021", add(_var_1020, _var_1021))',
          '                                                                       ^^^^^^^^^^^^^^^^^^^^^^^')

    check($rdb.groupedmapreduce(lambda {|x| x[:id] % "a"}, lambda {|x| x[:id]}, 0, lambda {|a,b| a+b}),
          'Query: db("test").table("tbl").groupedmapreduce(["_var_1030", modulo(_var_1030[:id], "a")], ["_var_1031", _var_1031[:id]], [0, "_var_1032", "_var_1033", add(_var_1032, _var_1033)])',
          '                                                                                     ^^^')
    check($rdb.groupedmapreduce(lambda {|x| x[:id] % 4}, lambda {|x| x[:id]+"a"}, 0, lambda {|a,b| a+b}),
          'Query: db("test").table("tbl").groupedmapreduce(["_var_1034", modulo(_var_1034[:id], 4)], ["_var_1035", add(_var_1035[:id], "a")], [0, "_var_1036", "_var_1037", add(_var_1036, _var_1037)])',
          '                                                                                                                            ^^^')
    check($rdb.groupedmapreduce(lambda {|x| x[:id] % 4}, lambda {|x| x[:id]}, r.add(1,"a"), lambda {|a,b| a+b}),
          'Query: db("test").table("tbl").groupedmapreduce(["_var_1038", modulo(_var_1038[:id], 4)], ["_var_1039", _var_1039[:id]], [add(1, "a"), "_var_1040", "_var_1041", add(_var_1040, _var_1041)])',
          '                                                                                                                                 ^^^')
    check($rdb.groupedmapreduce(lambda {|x| x[:id] % 4}, lambda {|x| x[:id]}, 0, lambda {|a,b| a+b+"a"}),
          'Query: db("test").table("tbl").groupedmapreduce(["_var_1042", modulo(_var_1042[:id], 4)], ["_var_1043", _var_1043[:id]], [0, "_var_1044", "_var_1045", add(add(_var_1044, _var_1045), "a")])',
          '                                                                                                                                                                                      ^^^')
  end

  def test_meta
    check(r.db('test').create_table('tbl'),
          'Query: db("test").create_table("tbl")',
          '       ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^')
    check(r.db('test').drop_table('fake'),
          'Query: db("test").drop_table("fake")',
          '       ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^')
    check(r.db('a').create_table(''),
          'Query: db("a").create_table("")',
          '       ^^^^^^^^^^^^^^^^^^^^^^^^')
    check(r.db('a').drop_table(''),
          'Query: db("a").drop_table("")',
          '       ^^^^^^^^^^^^^^^^^^^^^^')
    check(r.db('a').list_tables,
          'Query: db("a").list_tables',
          '       ^^^^^^^^^^^^^^^^^^^')
  end

  def test_let
    check(r.let(:a => 1){r.letvar('b')},
          'Query: let([["a", 1]], letvar("b"))',
          '                       ^^^^^^^^^^^')
    check(r.let({:a => r.add(1, "a")}){r.letvar('a')},
          'Query: let([["a", add(1, "a")]], letvar("a"))',
          '                         ^^^')
  end

  def test_between
    check($rdb.between({}, 4),
          'Query: db("test").table("tbl").range(:id, {}, 4)',
          '                                          ^^')
    check($rdb.between(0, {}),
          'Query: db("test").table("tbl").range(:id, 0, {})',
          '                                             ^^')
    check($rdb.between(r.add(1,"a"), r.add(1,"a")),
          'Query: db("test").table("tbl").range(:id, add(1, "a"), add(1, "a"))',
          '                                                 ^^^')
    check($rdb.between(0, r.add(1,"a")),
          'Query: db("test").table("tbl").range(:id, 0, add(1, "a"))',
          '                                                    ^^^')
  end

  def test_streamops
    check($rdb.map{|x| r.add(1,x)},
          'Query: db("test").table("tbl").map("_var_1050", add(1, _var_1050))',
          '                                                       ^^^^^^^^^')
    check(r.db('a').table('b').update{{}},
          'Query: update(db("a").table("b"), ["_var_1007", {}])',
          '              ^^^^^^^^^^^^^^^^^^')
    check($rdb.update{{:new => r.add(1, "a")}},
          'Query: update(db("test").table("tbl"), ["_var_1052", {:new=>add(1, "a")}])',
          '                                                                   ^^^')
    check($rdb.update{{:id => -1}},
          'Query: update(db("test").table("tbl"), ["_var_1054", {:id=>-1}])',
          '                                                     ^^^^^^^^^')
    check(r.db('a').table('b').mutate{{}},
          'Query: mutate(db("a").table("b"), ["_var_1008", {}])',
          '              ^^^^^^^^^^^^^^^^^^')
    check($rdb.mutate{{:new => r.add(1, "a")}},
          'Query: mutate(db("test").table("tbl"), ["_var_1056", {:new=>add(1, "a")}])',
          '                                                                   ^^^')
    check($rdb.mutate{{:new => 1}},
          'Query: mutate(db("test").table("tbl"), ["_var_1056", {:new=>1}])',
          '                                                     ^^^^^^^^^')
    check($rdb.filter{1},
          'Query: db("test").table("tbl").filter("_var_1060", 1)',
          '                                                   ^')
    check($rdb.filter{$rdb},
          'Query: db("test").table("tbl").filter("_var_1061", db("test").table("tbl"))',
          '                                                   ^^^^^^^^^^^^^^^^^^^^^^^')
    check($rdb.filter{|row| r.eq(row[:id], r.add(1, "a"))},
          'Query: db("test").table("tbl").filter("_var_1062", compare(:eq, _var_1062[:id], add(1, "a")))',
          '                                                                                       ^^^')
    check($rdb.filter{|row| row[:id]},
          'Query: db("test").table("tbl").filter("_var_1002", _var_1002[:id])',
          '                                                   ^^^^^^^^^^^^^^')
  end

  def test_orderby
    check($rdb.orderby(:bah),
          'Query: db("test").table("tbl").orderby([:bah, true])',
          '       ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^')
  end

  def test_foreach
    check(r.db('a').table('b').foreach{|row| $rdb.get(0).update{{:new => row[:id]}}},
          'Query: foreach(db("a").table("b"), "_var_1066", [pointupdate(["test", "tbl"], :id, 0, ["_var_1067", {:new=>_var_1066[:id]}])])',
          '               ^^^^^^^^^^^^^^^^^^')
    check($rdb.foreach{|row| [$rdb.get(0).update{{:id => row[:id]}}, $rdb.mutate{|row| row}]},
          'Query: foreach(db("test").table("tbl"), "_var_1071", [pointupdate(["test", "tbl"], :id, 0, ["_var_1072", {:id=>_var_1071[:id]}]), mutate(db("test").table("tbl"), ["_var_1073", _var_1073])])',
          '                                                                                                         ^^^^^^^^^^^^^^^^^^^^^')
    check($rdb.foreach{|row| [$rdb.get(0).update{{:new => row[:id]}}, $rdb.mutate{|row| {}}]},
          'Query: foreach(db("test").table("tbl"), "_var_1001", [pointupdate(["test", "tbl"], :id, 0, ["_var_1002", {:new=>_var_1001[:id]}]), mutate(db("test").table("tbl"), ["_var_1003", {}])])',
          '                                                                                                                                                                                 ^^')
  end

  def test_obj_access
    check(r.expr({:a => r.add(1,"a")})[:b],
          'Query: {:a=>add(1, "a")}[:b]',
          '                   ^^^')
    check(r.expr({:a => 1})[:b],
          'Query: {:a=>1}[:b]',
          '       ^^^^^^^^^^^')
  end

  def test_insert
    $rdb.filter{|row| row[:id] < -1000}.delete.run
    check($rdb.insert([{:id => -1337}, {:id => 0}]),
          'Query: insert(["test", "tbl"], [{:id=>-1337}, {:id=>0}], false)',
          '                                              ^^^^^^^^')
    check($rdb.insert([{:id => {}}]),
          'Query: insert(["test", "tbl"], [{:id=>{}}], false)',
          '                                ^^^^^^^^^')
    check(r.db('a').table('b').insert({:id => -1337}),
          'Query: insert(["a", "b"], [{:id=>-1337}], false)',
          '       ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^')
    check($rdb.insert(r.expr([{:id => {}}]).to_stream),
          'Query: insert(["test", "tbl"], [[{:id=>{}}].arraytostream()], false)',
          '                                ^^^^^^^^^^^^^^^^^^^^^^^^^^^')
  end
end
