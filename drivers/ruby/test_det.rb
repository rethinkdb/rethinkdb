# Copyright 2010-2012 RethinkDB, all rights reserved.
class DetTest < Test::Unit::TestCase
  include RethinkDB::Shortcuts
  include RethinkDB::TestHelper

  def server_data
    data_table.order_by(:id).run.to_a
  end

  def test_det
    res = data_table.update{|row| {:count => r.js('0')}}.run
    assert_equal(res['errors'], 10); assert_not_nil(res['first_error'])
    res = data_table.update{|row| {:count => 0}}.run
    assert_equal(res, {'updated'=>10, 'errors'=>0, 'skipped'=>0})

    res = data_table.replace{|row| data_table.get(row[:id])}.run
    assert_equal(res['errors'], 10); assert_not_nil(res['first_error'])
    res = data_table.replace{|row| row}.run

    res = data_table.update{{:count => data_table.map{|x| x[:count]}.reduce(0){|a,b| a+b}}}.run
    assert_equal(res['errors'], 10); assert_not_nil(res['first_error'])
    static = r.expr(server_data)
    res = data_table.update{{:count => static.map{|x| x[:id]}.reduce(0){|a,b| a+b}}}.run
    assert_equal(res, {'skipped'=>0, 'updated'=>10, 'errors'=>0})

    assert_equal(data_table.map{|row| row[:count]}.reduce(0){|a,b| a+b}.run,
                 (data_table.map{|row| row[:id]}.reduce(0){|a,b| a+b} * data_table.count).run)

    # UPDATE MODIFY
    res = data_table.update{|row| {:x => r.js('1')}}.run
    assert_equal(res['errors'], 10); assert_not_nil(res['first_error'])
    res = data_table.update(:non_atomic){|row| {:x => r.js('1')}}.run
    assert_equal(res, {"skipped"=>0, "errors"=>0, "updated"=>10})
    assert_equal(data_table.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 10)

    assert_raise(RuntimeError){data_table.get(0).update{|row| {:x => r.js('1')}}.run}
    res = data_table.get(0).update(:non_atomic){|row| {:x => r.js('2')}}.run
    assert_equal(res, {'skipped'=>0, 'updated'=>1, 'errors'=>0})
    assert_equal(data_table.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 11)

    # UPDATE ERROR
    res = data_table.update{|row| {:x => r.js('x')}}.run
    assert_equal(res['errors'], 10); assert_not_nil(res['first_error'])
    res = data_table.update(:non_atomic){|row| {:x => r.js('x')}}.run
    assert_equal(res['errors'], 10); assert_not_nil(res['first_error'])
    assert_equal(data_table.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 11)

    assert_raise(RuntimeError){data_table.get(0).update{|row| {:x => r.js('x')}}.run}
    assert_raise(RuntimeError){data_table.get(0).update(:non_atomic){|row| {:x => r.js('x')}}.run}
    assert_equal(data_table.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 11)

    #UPDATE SKIPPED
    res = data_table.update{|row| r.branch(r.js('true'), nil, {:x => 0.1})}.run
    assert_equal(res['errors'], 10); assert_not_nil(res['first_error'])
    res = data_table.update(:non_atomic){|row| r.branch(r.js('true'), nil, {:x => 0.1})}.run
    assert_equal(res, {"skipped"=>10, "errors"=>0, "updated"=>0})
    assert_equal(data_table.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 11)

    assert_raise(RuntimeError){data_table.get(0).update{
        r.branch(r.js('true'), nil, {:x => 0.1})}.run}
    res = data_table.get(0).update(:non_atomic){r.branch(r.js('true'), nil, {:x => 0.1})}.run
    assert_equal(res, {'skipped'=>1, 'updated'=>0, 'errors'=>0})
    assert_equal(data_table.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 11)

    # MUTATE MODIFY
    assert_raise(RuntimeError){data_table.get(0).replace{|row| r.branch(r.js('true'), row, nil)}.run}
    res = data_table.get(0).replace(:non_atomic){|row| r.branch(r.js('true'),row,nil)}.run
    assert_equal(res, {"modified"=>1, "errors"=>0, "inserted"=>0, "deleted"=>0})
    assert_equal(data_table.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 11)

    res = data_table.replace{|row| r.branch(r.js("#{row}.id == 1"),row.merge({:x => 2}), row)}.run
    assert_equal(res['errors'], 10); assert_not_nil(res['first_error'])
    res = data_table.replace(:non_atomic){|row|
      r.branch(r.js("#{row}.id == 1"), row.merge({:x => 2}), row)}.run
    assert_equal(res, {"modified"=>10, "errors"=>0, "inserted"=>0, "deleted"=>0})
    assert_equal(data_table.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 12)

    #MUTATE ERROR
    assert_raise(RuntimeError){data_table.get(0).replace{|row| r.branch(r.js('x'), row, nil)}.run}
    assert_raise(RuntimeError){
      data_table.get(0).replace(:non_atomic){|row| r.branch(r.js('x'),row,nil)}.run}
    assert_equal(data_table.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 12)

    res = data_table.replace{|row| r.branch(r.js('x'), row, nil)}.run
    assert_equal(res['errors'], 10); assert_not_nil(res['first_error'])
    res = data_table.replace(:non_atomic){|row| r.branch(r.js('x'),row,nil)}.run
    assert_equal(res['errors'], 10); assert_not_nil(res['first_error'])
    assert_equal(data_table.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 12)

    #MUTATE DELETE
    assert_raise(RuntimeError){data_table.get(0).replace{|row| r.branch(r.js('true'),nil,row)}.run}
    res = data_table.get(0).replace(:non_atomic){|row| r.branch(r.js('true'),nil,row)}.run
    assert_equal(res, {'deleted'=>1, 'errors'=>0, 'inserted'=>0, 'modified'=>0})
    assert_equal(data_table.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 10)

    res = data_table.replace{|row| r.branch(r.js("#{row}.id < 3"), nil, row)}.run
    assert_equal(res['errors'], 9); assert_not_nil(res['first_error'])
    res = data_table.replace(:non_atomic){|row| r.branch(r.js("#{row}.id < 3"), nil, row)}.run
    assert_equal(res, {'inserted'=>0, 'deleted'=>2, 'errors'=>0, 'modified'=>7})
    assert_equal(data_table.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 7)

    #MUTATE INSERT
    assert_raise(RuntimeError){data_table.get(0).replace{
        {:id => 0, :count => data_table.get(3)[:count], :x => data_table.get(3)[:x]}}.run}
    res = data_table.get(0).replace(:non_atomic){
      {:id => 0, :count => data_table.get(3)[:count], :x => data_table.get(3)[:x]}}.run
    assert_equal(res, {"modified"=>0, "errors"=>0, "deleted"=>0, "inserted"=>1})
    assert_raise(RuntimeError){data_table.get(1).replace{data_table.get(3).merge({:id => 1})}.run}
    res = data_table.get(1).replace(:non_atomic){data_table.get(3).merge({:id => 1})}.run
    assert_equal(res, {"modified"=>0, "errors"=>0, "deleted"=>0, "inserted"=>1})
    res = data_table.get(2).replace(:non_atomic){data_table.get(1).merge({:id => 2})}.run
    assert_equal(res, {"modified"=>0, "errors"=>0, "deleted"=>0, "inserted"=>1})
    assert_equal(data_table.map{|row| row[:x]}.reduce(0){|a,b| a+b}.run, 10)
  end

end
