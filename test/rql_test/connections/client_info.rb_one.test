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

def test_client_port_and_address(conn)
  expect_ne(conn.client_port, nil)
  expect_ne(conn.client_address, nil)

  conn.close()

  expect_eq(conn.client_port, nil)
  expect_eq(conn.client_address, nil)
end

$tests = {
  :test_client_port_and_address => method(:test_client_port_and_address)
}

$tests.each do |name, m|
  timeout(10) {
    puts "Running test #{name}"
    c = r.connect(:host => 'localhost', :port => $port)
    m.call(c)
    puts " - PASS"
  }
end
