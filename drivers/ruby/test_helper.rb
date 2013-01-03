require 'rethinkdb'
require 'test/unit'

module RethinkDB::TestHelper
  def setup
    @c ||= RethinkDB::Connection.new('localhost', ARGV[0].to_i + 28015)

    unless r.db_list.run.include? 'test'
      r.db_create('test').run
    end

    unless r.db('test').table_list.run.include? 'tbl'
      RethinkDB::RQL.db('test').table_create('tbl').run
    end

    unless r.db('test').table_list.run.include? 'tbl_with_data'
      RethinkDB::RQL.db('test').table_create('tbl_with_data').run
    end

    tbl.delete.run
    tbl.insert(data).run

    data_table.delete.run
    data_table.insert(data).run
  end

  def tbl
    r.db('test').table('tbl')
  end

  def data_table
    r.db('test').table('tbl_with_data')
  end

  def c
    @c
  end

  def id_sort x
    x.sort_by do |y| 
      y['id']
    end
  end

  def data
    @data ||= (0..9).map do |i|
      { 'id' => i, 'num' => i, 'name' => i.to_s }
    end
  end
end
