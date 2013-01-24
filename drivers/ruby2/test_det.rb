# Copyright 2010-2012 RethinkDB, all rights reserved.
$LOAD_PATH.unshift('./rethinkdb')
require 'rethinkdb.rb'
require 'test/unit'
$port_base = ARGV[0].to_i # 0 if none given
class DetTest < Test::Unit::TestCase
  include RethinkDB::Shortcuts
  def rdb
    @@c.use('test')
    r.table('tbl')
  end
  @@c = RethinkDB::Connection.new('localhost', $port_base + 28015)
  def c; @@c; end
  def server_data; rdb.order_by(:id).run.to_a; end

  def test__init
    begin
      r.db('test').create_table('tbl').run
    rescue
    end
    $data = (0...10).map{|x| {'id' => x}}
    rdb.delete.run
    rdb.insert($data).run
  end

  def test_det
    res = rdb.update{|row| {:count => r.js('0')}}.run
    assert_equal(res['errors'], 10); assert_not_nil(res['first_error'])
    res = rdb.update{|row| {:count => 0}}.run
    assert_equal(res, {'updated'=>10, 'errors'=>0, 'skipped'=>0})

    res = rdb.replace{|row| rdb.get(row[:id])}.run
    assert_equal(res['errors'], 10); assert_not_nil(res['first_error'])
    res = rdb.replace{|row| row}.run

    res = rdb.update{{:count => rdb.map{|x| x[:count]}.reduce(0){|a,b| a+b}}}.run
    assert_equal(res['errors'], 10); assert_not_nil(res['first_error'])
    static = r.expr(server_data)
    res = rdb.update{{:count => static.map{|x| x[:id]}.reduce(0){|a,b| a+b}}}.run
    assert_equal(res, {'skipped'=>0, 'updated'=>10, 'errors'=>0})
  end

  def test_nonatomic
    # UPDATE MODIFY
    res = rdb.update{|row| {:x => r.js('1')}}.run
    assert_equal(res['errors'], 10); assert_not_nil(res['first_error'])
    res = rdb.update(:non_atomic){|row| {:x => r.js('1')}}.run
    assert_equal(res, {"skipped"=>0, "errors"=>0, "updated"=>10})
    assert_equal(rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 10)

    assert_raise(RuntimeError){rdb.get(0).update{|row| {:x => r.js('1')}}.run}
    res = rdb.get(0).update(:non_atomic){|row| {:x => r.js('2')}}.run
    assert_equal(res, {'skipped'=>0, 'updated'=>1, 'errors'=>0})
    assert_equal(rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 11)

    # UPDATE ERROR
    res = rdb.update{|row| {:x => r.js('x')}}.run
    assert_equal(res['errors'], 10); assert_not_nil(res['first_error'])
    res = rdb.update(:non_atomic){|row| {:x => r.js('x')}}.run
    assert_equal(res['errors'], 10); assert_not_nil(res['first_error'])
    assert_equal(rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 11)

    assert_raise(RuntimeError){rdb.get(0).update{|row| {:x => r.js('x')}}.run}
    assert_raise(RuntimeError){rdb.get(0).update(:non_atomic){|row| {:x => r.js('x')}}.run}
    assert_equal(rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 11)

    #UPDATE SKIPPED
    res = rdb.update{|row| r.branch(r.js('true'), nil, {:x => 0.1})}.run
    assert_equal(res['errors'], 10); assert_not_nil(res['first_error'])
    res = rdb.update(:non_atomic){|row| r.branch(r.js('true'), nil, {:x => 0.1})}.run
    assert_equal(res, {"skipped"=>10, "errors"=>0, "updated"=>0})
    assert_equal(rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 11)

    assert_raise(RuntimeError){rdb.get(0).update{
        r.branch(r.js('true'), nil, {:x => 0.1})}.run}
    res = rdb.get(0).update(:non_atomic){r.branch(r.js('true'), nil, {:x => 0.1})}.run
    assert_equal(res, {'skipped'=>1, 'updated'=>0, 'errors'=>0})
    assert_equal(rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 11)

    # MUTATE MODIFY
    assert_raise(RuntimeError){rdb.get(0).replace{|row| r.branch(r.js('true'), row, nil)}.run}
    res = rdb.get(0).replace(:non_atomic){|row| r.branch(r.js('true'),row,nil)}.run
    assert_equal(res, {"modified"=>1, "errors"=>0, "inserted"=>0, "deleted"=>0})
    assert_equal(rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 11)

    res = rdb.replace{|row| r.branch(r.js("#{row}.id == 1"),row.merge({:x => 2}), row)}.run
    assert_equal(res['errors'], 10); assert_not_nil(res['first_error'])
    res = rdb.replace(:non_atomic){|row|
      r.branch(r.js("#{row}.id == 1"), row.merge({:x => 2}), row)}.run
    assert_equal(res, {"modified"=>10, "errors"=>0, "inserted"=>0, "deleted"=>0})
    assert_equal(rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 12)

    #MUTATE ERROR
    assert_raise(RuntimeError){rdb.get(0).replace{|row| r.branch(r.js('x'), row, nil)}.run}
    assert_raise(RuntimeError){
      rdb.get(0).replace(:non_atomic){|row| r.branch(r.js('x'),row,nil)}.run}
    assert_equal(rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 12)

    res = rdb.replace{|row| r.branch(r.js('x'), row, nil)}.run
    assert_equal(res['errors'], 10); assert_not_nil(res['first_error'])
    res = rdb.replace(:non_atomic){|row| r.branch(r.js('x'),row,nil)}.run
    assert_equal(res['errors'], 10); assert_not_nil(res['first_error'])
    assert_equal(rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 12)

    #MUTATE DELETE
    assert_raise(RuntimeError){rdb.get(0).replace{|row| r.branch(r.js('true'),nil,row)}.run}
    res = rdb.get(0).replace(:non_atomic){|row| r.branch(r.js('true'),nil,row)}.run
    assert_equal(res, {'deleted'=>1, 'errors'=>0, 'inserted'=>0, 'modified'=>0})
    assert_equal(rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 10)

    res = rdb.replace{|row| r.branch(r.js("#{row}.id < 3"), nil, row)}.run
    assert_equal(res['errors'], 9); assert_not_nil(res['first_error'])
    res = rdb.replace(:non_atomic){|row| r.branch(r.js("#{row}.id < 3"), nil, row)}.run
    assert_equal(res, {'inserted'=>0, 'deleted'=>2, 'errors'=>0, 'modified'=>7})
    assert_equal(rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 7)

    #MUTATE INSERT
    assert_raise(RuntimeError){rdb.get(0).replace{
        {:id => 0, :count => rdb.get(3)[:count], :x => rdb.get(3)[:x]}}.run}
    res = rdb.get(0).replace(:non_atomic){
      {:id => 0, :count => rdb.get(3)[:count], :x => rdb.get(3)[:x]}}.run
    assert_equal(res, {"modified"=>0, "errors"=>0, "deleted"=>0, "inserted"=>1})
    assert_raise(RuntimeError){rdb.get(1).replace{rdb.get(3).merge({:id => 1})}.run}
    res = rdb.get(1).replace(:non_atomic){rdb.get(3).merge({:id => 1})}.run
    assert_equal(res, {"modified"=>0, "errors"=>0, "deleted"=>0, "inserted"=>1})
    res = rdb.get(2).replace(:non_atomic){rdb.get(1).merge({:id => 2})}.run
    assert_equal(res, {"modified"=>0, "errors"=>0, "deleted"=>0, "inserted"=>1})
    assert_equal(rdb.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 10)
  end

  def test_det_end
    assert_equal(rdb.map{|row| row[:count]}.reduce(0){|a,b| a+b}.run,
                 (rdb.map{|row| row[:id]}.reduce(0){|a,b| a+b} * rdb.count).run)
  end
end
