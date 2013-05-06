##
# Tests the driver API for making connections and excercizes the networking code
###

$LOAD_PATH.unshift '../../drivers/ruby/lib'
$LOAD_PATH.unshift '../../build/drivers/ruby/rdb_protocol'
require 'rethinkdb'
extend RethinkDB::Shortcuts

require 'rubygems'
gem 'test-unit'
require 'test/unit'

$server_build_dir = ARGV[0]
$use_default_port = ARGV[1].to_i == 1

class TestNoConnection < Test::Unit::TestCase

  include RethinkDB::Shortcuts

  # No servers started yet so this should fail
  if $use_default_port
    def test_connect
      assert_raise(Errno::ECONNREFUSED){ r.connect }
    end
  end

  def test_connect_port
    assert_raise(Errno::ECONNREFUSED){ r.connect '0.0.0.0', 11221 }
    if $use_default_port
      assert_raise(Errno::ECONNREFUSED){ r.connect '0.0.0.0' }
    end
  end

  def test_empty_run
    # Test the error message when we pass nothing to run and
    # didn't call `repl`
    assert_raise(ArgumentError){ r.expr(1).run }
  end
end

class RethinkDBServer
  include RethinkDB::Shortcuts

  attr_reader :conn
  attr_reader :port

  @@rdb_count = 0

  def initialize(port = 0)
    if port == 0 then
      @port = rand(65535-1024)+1024
    else
      @port = port
    end
    run_dir = "run/server_#{$$}"
    Dir.mkdir run_dir if not File.exist? run_dir
    index = @@rdb_count += 1
    @io = IO.popen("#{$server_build_dir}/rethinkdb --http-port 0 --cluster-port 0 --driver-port #{@port} 2>&1 --directory #{run_dir}/rdb_#{index}")
    lines = []
    @io.each do |line|
      lines << line
      if line =~ /Server ready/
        @conn = r.connect 'localhost', @port
        return
      end
    end
    lines.each{ |line| puts "#{line}" }
    raise Exception, "RethinkDB process failed to start correctly"
  end

  def query q
    q.run(@conn)
  end

  def close
    @conn.close
    Process.kill "INT", @io.pid
    lines = @io.readlines
    @io.close
    if $? != 0
      lines.each{ |line| puts line }
      raise Exception, "RethinkDB process returned non-zero exit status"
    end
  end
end

if $use_default_port
  class TestConnectionDefaultPort < Test::Unit::TestCase
    include RethinkDB::Shortcuts

    def setup
      @server = RethinkDBServer.new 28015
    end

    def teardown
      @server.close
    end

    def test_connect
      conn = r.connect
      conn.reconnect
    end

    def test_connect_host
      conn = r.connect 'localhost'
      conn.reconnect
    end

    def test_connect_host_port
      conn = r.connect 'localhost', 28015
      conn.reconnect()
    end
  end
end

class TestWithConnection < Test::Unit::TestCase
  include RethinkDB::Shortcuts

  def setup
    @server = RethinkDBServer.new
    @conn = @server.conn
  end

  def teardown
    if @server != nil
      @server.close
    end
  end

  def query q
    q.run @conn
  end
end

class TestConnection < TestWithConnection

  def test_connect_close_reconnect
    query r.expr(1)
    @conn.close
    @conn.close
    @conn.reconnect
    query r.expr(1)
  end

  def test_connect_close_expr
    query r.expr(1)
    @conn.close
    assert_raise(RuntimeError){ query r.expr(1) }
  end

  def test_db
    query r.db('test').table_create('t1')
    query r.db_create('db2')
    query r.db('db2').table_create('t2')

    # Default db should be 'test' so this will work
    query r.table('t1')

    # Use a new database
    @conn.use('db2')
    r.table('t2')
    assert_raise(RethinkDB::RqlRuntimeError){ query r.table('t1') }

    @conn.use('test')
    r.table('t1')
    assert_raise(RethinkDB::RqlRuntimeError){ query r.table('t2') }

    @conn.close

    # Test setting the db in connect
    @conn = r.connect 'localhost', @server.port, 'db2'
    query r.table('t2')

    assert_raise(Exception){ query r.table('t1') }

    @conn.close

    # Test setting the db as a `run` option
    @conn = r.connect 'localhost', @server.port
    r.table('t2').run(@conn, 'db2')
  end

  def test_use_outdated
    query r.db('test').table_create('t1')

    # Use outdated is an option that can be passed to db.table or `run`
    # We're just testing here if the server actually accepts the option.

    r.table('t1', { :use_outdated => true }).run(@conn)
    r.table('t1').run({ :connection => @conn, :use_outdated => true })
  end

  def test_repl

    # Calling .repl() should set this connection as global state
    # to be used when `run` is not otherwise passed a connection.
    c = @conn.repl

    r.expr(1).run()

    c.repl() # is idempotent

    r.expr(1).run()

    @conn.close()

    assert_raise(RuntimeError){ r.expr(1).run }
  end
end

class TestShutdown < TestWithConnection
  def test_shutdown
    query r.expr(1)
    @server.close
    assert_raise(RuntimeError){ query r.expr(1) }
  end
end


# This doesn't really have anything to do with connections but it'll go
# in here for the time being.
class TestPrinting < Test::Unit::TestCase

    # Just test that RQL queries support __str__ using the pretty printer.
    # An exhaustive test of the pretty printer would be, well, exhausing.
    def runTest
        assert_equal("#{r.db('db1').table('tbl1').map(lambda{ |x| x })}",
                     "r.db('db1').table('tbl1').map(lambda var_1: var_1)")
    end
end
