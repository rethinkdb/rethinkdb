#!/usr/bin/env ruby
# Copyright 2014-2016 RethinkDB, all rights reserved.

begin
    require 'minitest/autorun'
rescue LoadError
    require 'test/unit'
end

begin
  UNIT_TEST_CLASS = MiniTest::Test
rescue
  begin
    UNIT_TEST_CLASS = MiniTest::Unit::TestCase
  rescue
    UNIT_TEST_CLASS = Test::Unit::TestCase
  end
end

# -- import the rethinkdb driver

require_relative '../importRethinkDB.rb'

# --

$port = (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i

def expect_error(m, err_type, err_info)
  begin
    m.call()
  rescue err_type => ex
    raise if ex.message.lines.first.rstrip != err_info
    return
  end
  raise RuntimeError, "Expected an error, but got success with result: #{res}."
end

class Cursor_Test < UNIT_TEST_CLASS
  @conn = nil
  
  def setup
    if @conn.nil?
      @conn = r.connect(:host => 'localhost', :port => $port)
    else
      begin
        r.expr(1).run(@conn)
      rescue RethinkDB::ReqlError
        @conn.close rescue nil
        @conn = r.connect(:host => 'localhost', :port => $port)
      end
    end
  end

  def test_cursor_after_connection_close()
    cursor = r.range().run(@conn)
    @conn.close()
    count = 0
    read_cursor = -> {
      cursor.each{ |i| count += 1 }
    }
    expect_error(read_cursor, RethinkDB::ReqlRuntimeError, "Connection is closed.")
    raise "Did not get any cursor results" if count == 0
  end
  
  def test_cursor_after_cursor_close()
    cursor = r.range().run(@conn)
    cursor.close()
    count = 0
    cursor.each{ |i| count += 1 }
    raise "Did not get any cursor results" if count == 0
  end
  
  def test_cursor_close_in_each()
    cursor = r.range().run(@conn)
    count = 0
    cursor.each{ cursor.close() if (count += 1) == 2 }
    raise "Did not get any cursor results" if count <= 2
  end
  
  def test_cursor_success()
    cursor = r.range().limit(10000).run(@conn)
    count = 0
    cursor.each{ count += 1 }
    assert_equal(count, 10000)
  end
  
  def test_cursor_double_each()
    cursor = r.range().limit(10000).run(@conn)
    count = 0
    read_cursor = -> {
      cursor.each{ count += 1 }
    }
    read_cursor.call()
    assert_equal(count, 10000)
    expect_error(read_cursor, RethinkDB::ReqlRuntimeError, "Can only iterate over a cursor once.")
    assert_equal(count, 10000)
  end
  
  def get_next(cursor, wait_time)
    begin
      case wait_time
      when nil
        cursor.next()
      else
        cursor.next(wait_time)
      end
      return 1, 0
    rescue Timeout::Error => ex
      raise if ex.message != "Timed out waiting for cursor response."
      return 0, 1
    end
  end
  
  def cursor_wait_internal(wait_time)
    num_cursors = 3
    cursors = Array.new(num_cursors) {
      r.range().map(r.js("(function (row) {" +
                         "end = new Date(new Date().getTime() + 2);" +
                         "while (new Date() < end) { }" +
                         "return row;" +
                         "})")).run(@conn, :max_batch_rows => 500) }
    cursor_counts = Array.new(num_cursors) { 0 }
    cursor_timeouts = Array.new(num_cursors) { 0 }
  
    while cursor_counts.reduce(:+) < 4000
      (0...num_cursors).each do |index|
        (0...rand(10)).each do |x|
          reads, timeouts = get_next(cursors[index], wait_time)
          cursor_counts[index] += reads
          cursor_timeouts[index] += timeouts
        end
      end
    end
  
    cursors.each{ |x| x.close() }
    return cursor_counts.reduce(:+), cursor_timeouts.reduce(:+)
  end
  
  def test_cursor_false_wait()
    reads, timeouts = cursor_wait_internal(false)
    assert(timeouts > 0, "Did not get timeouts using zero (false) wait.")
  end
  
  def test_cursor_zero_wait()
    reads, timeouts = cursor_wait_internal(0)
    assert(timeouts > 0, "Did not get timeouts using zero wait.")
  end
  
  def test_cursor_short_wait()
    reads, timeouts = cursor_wait_internal(0.001)
    assert(timeouts > 0, "Did not get timeouts using short (1 millisecond) wait.")
  end
  
  def test_cursor_long_wait()
    reads, timeouts = cursor_wait_internal(10)
    assert_equal(timeouts, 0, "Got timeouts using long (10 second) wait.")
  end
  
  def test_cursor_infinite_wait()
    reads, timeouts = cursor_wait_internal(true)
    assert_equal(timeouts, 0, "Got timeouts using infinite wait.")
  end
  
  def test_cursor_default_wait()
    reads, timeouts = cursor_wait_internal(nil)
    assert_equal(timeouts, 0, "get timeouts using zero (false) wait.")
  end
  
  def test_changefeed_wait()
    dbName = 'cursor_rb'
    tableName = 'changefeed_wait'
    
    r.expr([dbName]).set_difference(r.db_list()).for_each{|row| r.db_create(row)}.run(@conn)
    r.expr([tableName]).set_difference(r.db(dbName).table_list()).for_each{|row| r.db(dbName).table_create(row)}.run(@conn)
  
    changes = r.db(dbName).table(tableName).changes().run(@conn)
  
    timeout = nil
    read_cursor = -> {
      changes.next(timeout)
    }
  
    timeout = 0
    expect_error(read_cursor, Timeout::Error, "Timed out waiting for cursor response.")
    timeout = 0.2
    expect_error(read_cursor, Timeout::Error, "Timed out waiting for cursor response.")
    timeout = 1
    expect_error(read_cursor, Timeout::Error, "Timed out waiting for cursor response.")
    timeout = 5
    expect_error(read_cursor, Timeout::Error, "Timed out waiting for cursor response.")
  
    res = r.db(dbName).table(tableName).insert({}).run(@conn)
    assert_equal(res['inserted'], 1)
    res = changes.next(wait=1)
  end
end
