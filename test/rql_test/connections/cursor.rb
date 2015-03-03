#!/usr/bin/env ruby

# -- import the rethinkdb driver

require_relative '../importRethinkDB.rb'

# --

$port = (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i

def expect_eq(left, right, message=nil)
    if left != right
        if message.nil?
            raise RuntimeError, "Actual value #{left} not equal to expected value #{right}."
        else
            raise RuntimeError, "Actual value #{left} not equal to expected value #{right}: #{message}"
        end
    end
end

def expect_ne(left, right, message=nil)
    if left == right
        if message.nil?
            raise RuntimeError, "Value #{left} unexpected."
        else
            raise RuntimeError, message
        end
    end
end

def expect_error(m, err_type, err_info)
    begin
        m.call()
    rescue err_type => ex
        raise if ex.message.lines.first.rstrip != err_info
        return
    end
    raise RuntimeError, "Expected an error, but got success with result: #{res}."
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

def get_next(cursor, wait_time)
    begin
        case wait_time
        when nil
            cursor.next()
        when false, 0
            # To avoid complete deadlock with 0 wait, occasionally allow more data through
            if rand() <= 0.99 then cursor.next(wait_time) else cursor.next(0.0001) end
        else
            cursor.next(wait_time)
        end
        return 1, 0
    rescue Timeout::Error => ex
        raise if ex.message != "Timed out waiting for cursor response."
        return 0, 1
    end
end

def test_cursor_wait_internal(conn, wait_time)
    num_cursors = 3
    cursors = Array.new(num_cursors) {
        r.range().map(r.js("(function (row) {" +
                               "end = new Date(new Date().getTime() + 2);" +
                               "while (new Date() < end) { }" +
                               "return row;" +
                           "})")).run(conn, :max_batch_rows => 2) }
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

def test_cursor_false_wait(conn)
    reads, timeouts = test_cursor_wait_internal(conn, false)
    expect_ne(timeouts, 0, "Did not get timeouts using zero (false) wait.")
end

def test_cursor_zero_wait(conn)
    reads, timeouts = test_cursor_wait_internal(conn, 0)
    expect_ne(timeouts, 0, "Did not get timeouts using zero wait.")
end

def test_cursor_short_wait(conn)
    reads, timeouts = test_cursor_wait_internal(conn, 0.001)
    expect_ne(timeouts, 0, "Did not get timeouts using short (1 millisecond) wait.")
end

def test_cursor_long_wait(conn)
    reads, timeouts = test_cursor_wait_internal(conn, 10)
    expect_eq(timeouts, 0, "Got timeouts using long (10 second) wait.")
end

def test_cursor_infinite_wait(conn)
    reads, timeouts = test_cursor_wait_internal(conn, true)
    expect_eq(timeouts, 0, "Got timeouts using infinite wait.")
end

def test_cursor_default_wait(conn)
    reads, timeouts = test_cursor_wait_internal(conn, nil)
    expect_eq(timeouts, 0, "get timeouts using zero (false) wait.")
end

$tests = {
    :test_cursor_after_connection_close => method(:test_cursor_after_connection_close),
    :test_cursor_after_cursor_close => method(:test_cursor_after_cursor_close),
    :test_cursor_close_in_each => method(:test_cursor_close_in_each),
    :test_cursor_success => method(:test_cursor_success),
    :test_cursor_double_each => method(:test_cursor_double_each),
    :test_cursor_false_wait => method(:test_cursor_false_wait),
    :test_cursor_zero_wait => method(:test_cursor_zero_wait),
    :test_cursor_short_wait => method(:test_cursor_short_wait),
    :test_cursor_long_wait => method(:test_cursor_long_wait),
    :test_cursor_infinite_wait => method(:test_cursor_infinite_wait),
    :test_cursor_default_wait => method(:test_cursor_default_wait),
}

$tests.each do |name, m|
     puts "Running test #{name}"
     c = r.connect(:host => 'localhost', :port => $port)
     m.call(c)
     c.close()
     puts " - PASS"
end

