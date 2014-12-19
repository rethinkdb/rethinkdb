#!/usr/bin/env ruby

# -- import the rethinkdb driver

require_relative '../importRethinkDB.rb'

# --

$port = (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i

def expect_eq(left, right)
    if left != right
        raise RuntimeError, "Actual value #{left} not equal to expected value #{right}"
    end
end

def expect_error(m, err_type, err_info)
    begin
        m.call()
    rescue err_type => ex
        raise if ex.message.lines.first.rstrip != err_info
        return
    end
    raise RuntimeError, "Expected an error, but got success with result: #{res}"
end

def test_cursor_after_connection_close(conn)
    cursor = r.range().run(conn)
    conn.close()
    count = 0
    read_cursor = -> {
        cursor.each{ |i| count += 1 }
    }
    expect_error(read_cursor, RethinkDB::RqlRuntimeError, "Connection is closed.")
    raise "Did not get any cursor results" if count == 0
end

def test_cursor_after_cursor_close(conn)
    cursor = r.range().run(conn)
    cursor.close()
    count = 0
    cursor.each{ |i| count += 1 }
    raise "Did not get any cursor results" if count == 0
end

def test_cursor_close_in_each(conn)
    cursor = r.range().run(conn)
    count = 0
    cursor.each{ cursor.close() if (count += 1) == 2 }
    raise "Did not get any cursor results" if count <= 2
end

def test_cursor_success(conn)
    cursor = r.range().limit(10000).run(conn)
    count = 0
    cursor.each{ count += 1 }
    expect_eq(count, 10000)
end

def test_cursor_double_each(conn)
    cursor = r.range().limit(10000).run(conn)
    count = 0
    read_cursor = -> {
        cursor.each{ count += 1 }
    }
    read_cursor.call()
    expect_eq(count, 10000)
    expect_error(read_cursor, RethinkDB::RqlRuntimeError, "Can only iterate over a cursor once.")
    expect_eq(count, 10000)
end

$tests = {
    :test_cursor_after_connection_close => method(:test_cursor_after_connection_close),
    :test_cursor_after_cursor_close => method(:test_cursor_after_cursor_close),
    :test_cursor_close_in_each => method(:test_cursor_close_in_each),
    :test_cursor_success => method(:test_cursor_success),
    :test_cursor_double_each => method(:test_cursor_double_each),
}

$tests.each do |name, m|
     puts "Running test #{name}"
     c = r.connect(:host => 'localhost', :port => $port)
     m.call(c)
     c.close()
     puts " - PASS"
end

