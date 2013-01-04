# Copyright 2010-2012 RethinkDB, all rights reserved.
class DetTest < Test::Unit::TestCase
  include RethinkDB::Shortcuts
  include RethinkDB::TestHelper

  def server_data
    data_table.order_by(:id).run.to_a
  end

  def test_det
    res = data_table.update { |row| { :count => (r.js("0")) } }.run
    assert_equal(10, res["errors"])
    assert_not_nil(res["first_error"])
    res = data_table.update { |row| { :count => 0 } }.run

    assert_equal({ "updated" => 10, "errors" => 0, "skipped" => 0 }, res)
    res = data_table.replace { |row| data_table.get(row[:id]) }.run
    assert_equal(10, res["errors"])

    assert_not_nil(res["first_error"])
    res = data_table.replace { |row| row }.run
    res = data_table.update do
      { :count => (data_table.map { |x| x[:count] }.reduce(0) { |a, b| (a + b) }) }
    end.run
    assert_equal(10, res["errors"])
    assert_not_nil(res["first_error"])
    static = r.expr(server_data)
    res = data_table.update do
      { :count => (static.map { |x| x[:id] }.reduce(0) { |a, b| (a + b) }) }
    end.run
    assert_equal({ "skipped" => 0, "updated" => 10, "errors" => 0 }, res)

    assert_equal((data_table.map { |row| row[:id] }.reduce(0) { |a, b| (a + b) } * data_table.count).run, data_table.map { |row| row[:count] }.reduce(0) { |a, b| (a + b) }.run)

    # UPDATE MODIFY
    res = data_table.update { |row| { :x => (r.js("1")) } }.run
    assert_equal(10, res["errors"])
    assert_not_nil(res["first_error"])
    res = data_table.update(:non_atomic) { |row| { :x => (r.js("1")) } }.run
    assert_equal({ "skipped" => 0, "errors" => 0, "updated" => 10 }, res)
    assert_equal(10, data_table.map { |row| row[:x] }.reduce(0) { |a, b| (a + b) }.run)

    assert_raise(RuntimeError) do
      data_table.get(0).update { |row| { :x => (r.js("1")) } }.run
    end
    res = data_table.get(0).update(:non_atomic) { |row| { :x => (r.js("2")) } }.run
    assert_equal({ "skipped" => 0, "updated" => 1, "errors" => 0 }, res)
    assert_equal(11, data_table.map { |row| row[:x] }.reduce(0) { |a, b| (a + b) }.run)

    # UPDATE ERROR
    res = data_table.update { |row| { :x => (r.js("x")) } }.run
    assert_equal(10, res["errors"])
    assert_not_nil(res["first_error"])
    res = data_table.update(:non_atomic) { |row| { :x => (r.js("x")) } }.run
    assert_equal(10, res["errors"])
    assert_not_nil(res["first_error"])
    assert_equal(11, data_table.map { |row| row[:x] }.reduce(0) { |a, b| (a + b) }.run)

    assert_raise(RuntimeError) do
      data_table.get(0).update { |row| { :x => (r.js("x")) } }.run
    end
    assert_raise(RuntimeError) do
      data_table.get(0).update(:non_atomic) { |row| { :x => (r.js("x")) } }.run
    end
    assert_equal(11, data_table.map { |row| row[:x] }.reduce(0) { |a, b| (a + b) }.run)

    #UPDATE SKIPPED
    res = data_table.update { |row| r.branch(r.js("true"), nil, :x => 0.1) }.run
    assert_equal(10, res["errors"])
    assert_not_nil(res["first_error"])
    res = data_table.update(:non_atomic) do |row|
      r.branch(r.js("true"), nil, :x => 0.1)
    end.run
    assert_equal({ "skipped" => 10, "errors" => 0, "updated" => 0 }, res)
    assert_equal(11, data_table.map { |row| row[:x] }.reduce(0) { |a, b| (a + b) }.run)

    assert_raise(RuntimeError) do
      data_table.get(0).update { r.branch(r.js("true"), nil, :x => 0.1) }.run
    end
    res = data_table.get(0).update(:non_atomic) do
      r.branch(r.js("true"), nil, :x => 0.1)
    end.run
    assert_equal({ "skipped" => 1, "updated" => 0, "errors" => 0 }, res)
    assert_equal(11, data_table.map { |row| row[:x] }.reduce(0) { |a, b| (a + b) }.run)

    # MUTATE MODIFY
    assert_raise(RuntimeError) do
      data_table.get(0).replace { |row| r.branch(r.js("true"), row, nil) }.run
    end
    res = data_table.get(0).replace(:non_atomic) do |row|
      r.branch(r.js("true"), row, nil)
    end.run
    assert_equal({ "modified" => 1, "errors" => 0, "inserted" => 0, "deleted" => 0 }, res)
    assert_equal(11, data_table.map { |row| row[:x] }.reduce(0) { |a, b| (a + b) }.run)

    res = data_table.replace do |row|
      r.branch(r.js("#{row}.id == 1"), row.merge(:x => 2), row)
    end.run
    assert_equal(10, res["errors"])
    assert_not_nil(res["first_error"])
    res = data_table.replace(:non_atomic) do |row|
      r.branch(r.js("#{row}.id == 1"), row.merge(:x => 2), row)
    end.run
    assert_equal({ "modified" => 10, "errors" => 0, "inserted" => 0, "deleted" => 0 }, res)
    assert_equal(12, data_table.map { |row| row[:x] }.reduce(0) { |a, b| (a + b) }.run)

    # MUTATE ERROR
    assert_raise(RuntimeError) do
      data_table.get(0).replace { |row| r.branch(r.js("x"), row, nil) }.run
    end
    assert_raise(RuntimeError) do
      data_table.get(0).replace(:non_atomic) do |row|
        r.branch(r.js("x"), row, nil)
      end.run
    end
    assert_equal(12, data_table.map { |row| row[:x] }.reduce(0) { |a, b| (a + b) }.run)
    res = data_table.replace { |row| r.branch(r.js("x"), row, nil) }.run
    assert_equal(10, res["errors"])
    assert_not_nil(res["first_error"])
    res = data_table.replace(:non_atomic) { |row| r.branch(r.js("x"), row, nil) }.run
    assert_equal(10, res["errors"])
    assert_not_nil(res["first_error"])
    assert_equal(12, data_table.map { |row| row[:x] }.reduce(0) { |a, b| (a + b) }.run)

    # MUTATE DELETE
    assert_raise(RuntimeError) do
      data_table.get(0).replace { |row| r.branch(r.js("true"), nil, row) }.run
    end
    res = data_table.get(0).replace(:non_atomic) do |row|
      r.branch(r.js("true"), nil, row)
    end.run
    assert_equal({ "deleted" => 1, "errors" => 0, "inserted" => 0, "modified" => 0 }, res)
    assert_equal(10, data_table.map { |row| row[:x] }.reduce(0) { |a, b| (a + b) }.run)

    res = data_table.replace { |row| r.branch(r.js("#{row}.id < 3"), nil, row) }.run
    assert_equal(9, res["errors"])
    assert_not_nil(res["first_error"])
    res = data_table.replace(:non_atomic) do |row|
      r.branch(r.js("#{row}.id < 3"), nil, row)
    end.run
    assert_equal({ "inserted" => 0, "deleted" => 2, "errors" => 0, "modified" => 7 }, res)
    assert_equal(7, data_table.map { |row| row[:x] }.reduce(0) { |a, b| (a + b) }.run)

    # MUTATE INSERT
    assert_raise(RuntimeError) do
      data_table.get(0).replace do
        { :id => 0, :count => (data_table.get(3)[:count]), :x => (data_table.get(3)[:x]) }
      end.run
    end
    res = data_table.get(0).replace(:non_atomic) do
      { :id => 0, :count => (data_table.get(3)[:count]), :x => (data_table.get(3)[:x]) }
    end.run
    assert_equal({ "modified" => 0, "errors" => 0, "deleted" => 0, "inserted" => 1 }, res)
    assert_raise(RuntimeError) do
      data_table.get(1).replace { data_table.get(3).merge(:id => 1) }.run
    end
    res = data_table.get(1).replace(:non_atomic) { data_table.get(3).merge(:id => 1) }.run
    assert_equal({ "modified" => 0, "errors" => 0, "deleted" => 0, "inserted" => 1 }, res)
    res = data_table.get(2).replace(:non_atomic) { data_table.get(1).merge(:id => 2) }.run
    assert_equal({ "modified" => 0, "errors" => 0, "deleted" => 0, "inserted" => 1 }, res)
    assert_equal(10, data_table.map { |row| row[:x] }.reduce(0) { |a, b| (a + b) }.run)
  end
end
