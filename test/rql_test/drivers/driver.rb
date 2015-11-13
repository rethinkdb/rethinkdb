require 'pp'
require 'set'

$test_count = 0
$success_count = 0

DEBUG_ENABLED = ENV['VERBOSE'] ? ENV['VERBOSE'] == 'true' : false
START_TIME = Time.now()

def print_debug(message)
    if DEBUG_ENABLED
        puts("DEBUG %.2f:\t %s" % [Time.now() - START_TIME, message])
    end
end

DRIVER_PORT = (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i
print_debug("Using driver port #{DRIVER_PORT}")

$required_external_tables = []
if ARGV[1] || ENV['TEST_DB_AND_TABLE_NAME']
  (ARGV[1] || ENV['TEST_DB_AND_TABLE_NAME']).split(',').each { |table_raw|
    table_raw = table_raw.strip()
    if table_raw == ''
      next
    end
    split_value = table_raw.split('.')
    case split_value.count
    when 1
      $required_external_tables.push(['test', split_value[0]])
    when 2
      $required_external_tables.push([split_value[0], split_value[1]])
    else
      raise('Unusable value for external tables: ' + table_raw)
    end
  }
end
$required_external_tables = $required_external_tables.reverse
$stdout.flush

# -- import the rethinkdb driver

require_relative '../importRethinkDB.rb'

# --

$reql_conn = RethinkDB::Connection.new(:host => 'localhost', :port => DRIVER_PORT)
begin
  r.db_create('test').run($reql_conn)
rescue
end

# --

$defines = binding
$defines.eval('conn = $reql_conn') # allow access to connection

NoError = "<no error>"
AnyUUID = "<any uuid>"
Err = Struct.new(:class, :message, :backtrace)
Bag = Struct.new(:value, :ordered, :partial)

# --

def show(x)
  if x.is_a?(Err)
    return "#{x.class.name.sub(/^RethinkDB::/, "")}: #{'~ ' if x.message.is_a? Regexp}#{show x.message}"
  elsif x.is_a?(Exception)
    message = String.new(x.message)
    message.sub!(/^(?<message>[^\n]*?)\nFailed assertion:.*$/m, '\k<message>')
    message.sub!(/^(?<message>[^\n]*?)\nBacktrace:\n.*$/m, '\k<message>')
    message.sub!(/^(?<message>[^\n]*?):\n.*$/m, '\k<message>:')
    return "#{x.class.name.sub(/^RethinkDB::/, "")}: #{message}"
  elsif x.is_a?(String)
    return x
  end
  return (PP.pp x, "").chomp
end

def bag(expected, ordered=nil, partial=nil)
  if ordered.nil?
    ordered = false
  end
  if partial.nil?
    partial = false
  end
  
  if expected.kind_of?(Array) or expected.kind_of?(Hash)
    Bag.new(expected, ordered, partial)
  elsif expected.kind_of?(Bag)
    Bag.new(expected.items, ordered, partial)
  else
    raise("bag() can only handle items ot type Hash, Array, or Bag. Got: #{expected.class}")
  end
end

def partial(expected, ordered=nil, partial=nil)
  if ordered.nil?
    ordered = false
  end
  if partial.nil?
    partial = true
  end
  
  if expected.kind_of?(Array) or expected.kind_of?(Hash)
    Bag.new(expected, ordered, partial)
  elsif expected.kind_of?(Bag)
    Bag.new(expected.items, ordered, partial)
  else
    raise("partial() can only handle items ot type Hash, Array, or Bag. Got: #{expected.class}")
  end
end

def fetch(cursor, limit=nil)
  raise "The limit value of fetch must be nil or > 0, got: #{limit}" unless limit.nil? or true
  if limit.nil?
    return cursor.to_a
  else
    result = []
    limit.times do
      result.push(cursor.next)
    end
    return result
  end
end

def wait(seconds)
  sleep(seconds)
end

def arrlen(len, x)
  Array.new(len, x)
end

def uuid
  AnyUUID
end

def regex(pattern)
  Regexp.new(pattern)
end

def shard
  # do nothing in Ruby tests
end

def err(type, message, backtrace=[])
  Err.new(RethinkDB.const_get(type), message, backtrace)
end

def err_regex(type, message, backtrace=[])
  Err.new(RethinkDB.const_get(type), Regexp.new(message), backtrace)
end

class Number
  def initialize(value)
    @value = value
    @requiredType = value.class
  end

  def method_missing(name, *args)
    return @number.send(name, *args)
  end

  def to_s
    return @value.to_s + "(explicit" + @requiredType.name + ")"
  end

  def inspect
    return @value.to_s + " (explicit " + @requiredType.name + ")"
  end
  attr_reader :value
  attr_reader :requiredType
end

def int_cmp(value)
  raise "int_cmp got a non Fixnum input: " + value.to_s unless value.kind_of?(Fixnum)
  return Number.new(value)
end

def float_cmp(value)
  raise "int_cmp got a non Float input: " + value.to_s unless value.kind_of?(Float)
  return Number.new(value)
end

def cmp_test(expected, result, testopts={}, ordered=true, partial=false)
  print_debug("\tCompare - expected: <<#{show(expected)}>> (#{expected.class}) actual: <<#{show(result)}>> (#{result.class})")
  
  # - NoError
  if expected.object_id == NoError.object_id
    if result.is_a?(Err) || result.is_a?(Exception) || result.is_a?(RethinkDB::ReqlError)
      puts result
      puts result.backtrace
      return -1
    end
    return 0
  end
  
  # - nils in expected or result
  if expected.nil? and result.nil?
    return 0
  elsif result.nil?
    return 1
  elsif expected.nil?
    return -1
  end
  
  # - AnyUUID
  if expected.object_id == AnyUUID.object_id
    return -1 if not result.kind_of? String
    return 0 if result.match /[a-z0-9]{8}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{12}/
    return 1
  end
  
  # - unpack Bags
  if expected.is_a?(Bag)
    ordered = expected.ordered
    partial = expected.partial
    expected = expected.value
  end
  
  # - strip assertion messages from any strings
  if result.is_a?(String) then
    result = result.sub(/\nFailed assertion:(.|\n)*/, "")
  end
  
  # -
  
  case
  when expected.is_a?(Err) || expected.is_a?(Exception)
    if result.is_a?(expected.class)
      cmpMessage = show(result).sub(/^#{result.class.name.sub(/^RethinkDB::/, '')}: /, '')
      if expected.message.is_a?(Regexp)
        return expected.message.match(cmpMessage) ? 0 : 1 
      else
        return cmpMessage <=> expected.message
      end
    else
      return result.class.name <=> expected.class.name
    end
  
  when expected.is_a?(Regexp)
    # short circuit if result is not a String
    if not result.is_a?(String)
      return -1
    end
    return expected.match(result) ? 0 : 1
  
  when expected.is_a?(Array)
    # short circuit if result is not an array
    if not result.is_a?(Array)
      return -1
    end
    
    # short circut on lengths
    if expected.length == 0
      if partial or result.length == 0
        return 0
      else
        return -1
      end
    end
    if not(partial) && expected.length != result.length
      return -1
    end
    
    if ordered
      haystack = result.each
      for needle in expected
        begin
          straw = haystack.next
        rescue StopIteration
          return -1 # ran out of straw before finding all the needles
        end
        while cmp_test(needle, straw, testopts=testopts, ordered=ordered, partial=partial) != 0
          if not partial
            return -1
          end
          begin
            straw = haystack.next
          rescue StopIteration
            return -1 # ran out of straw before finding all the needles
          end
        end
      end
      if not partial
        begin
          haystack.next
        rescue StopIteration
          # looks good
        else
          return -1 # ran out of needles before straw
        end
      end
    else
      # non-ordered
      needles = expected.dup
      for straw in result
        if needles.length == 0
          if partial
            return 0
          else
            return -1 # ran out of needles before straw
          end
        end
        if needles.each do |needle|
          if cmp_test(needle, straw, testopts=testopts, ordered=ordered, partial=partial) == 0
            needles.delete_at(needles.find_index(needle))
            break needle # bypass the `end.nil?` below
          end
        end.nil?
          if not partial
            return -1 # no match for this straw
          end
        end
      end
      if needles.length != 0
        return -1
      end
    end
    return 0

  when expected.is_a?(Hash)
    if not result.is_a?(Hash)
      return -1 # short circuit if not a hash
    end
    
    # short circut on lengths
    if expected.length == 0
      if partial or result.length == 0
        return 0
      else
        return -1
      end
    end
    if not(partial) && expected.length != result.length
      return expected.length, result.length
    end
    
    # compare the items
    needles = expected.dup
    result.each{ |strawKey, strawValue|
      if needles.length == 0
        if partial
          return 0
        else
          return -1
        end
      end
      if needles.has_key?(strawKey)
        if cmp_test(needles[strawKey], strawValue, testopts=testopts, ordered=ordered, partial=partial) != 0
          return -1
        else
          needles.delete(strawKey)
        end
      elsif not partial
        return -1 # not everything in the hastack is a needle
      end
    }
    if needles.length != 0
      return -1
    end
    return 0
  
  when expected.is_a?(Float) || expected.is_a?(Fixnum) || expected.is_a?(Number)
    if not (result.kind_of? Float or result.kind_of? Fixnum)
      cmp = result.class.name <=> expected.class.name
      return cmp if cmp != 0
    end

    if expected.kind_of?(Number)
      if not result.kind_of?(expected.requiredType)
        cmp = result.class.name <=> expected.class.name
        return cmp if cmp != 0 else -1
      end
      expected = expected.value
    end

    if testopts.has_key?(:precision)
      diff = result - expected
      if (diff).abs < testopts[:precision]
        return 0
      else
        return diff <=> 0
      end
    else
      return result <=> expected
    end

  else
    begin
      cmp = result <=> expected
      return cmp if cmp != nil
      return result.class.name <=> expected.class.name
    rescue
      return result.class.name <=> expected.class.name
    end
  end
end

def test(src, expected, name, opthash=nil, testopts=nil)
  $test_count += 1

  begin
    # -- process the opthash

    if opthash
      opthash = Hash[opthash.map{ |key,value|
        if value.is_a?(String)
          begin
            value = $defines.eval(value)
          rescue NameError
          end
        end
        if key.is_a?(String)
          begin
            key = key.to_sym
          rescue NameError
          end
        end
        [key, value]
      }]
      if !opthash[:max_batch_rows] && !opthash['max_batch_rows']
        opthash[:max_batch_rows] = 3
      end
    else
      opthash = {:max_batch_rows => 3}
    end

    # -- process the testopts

    if testopts
      testopts = Hash[testopts.map{ |key,value|
        if value.is_a?(String)
          begin
            value = $defines.eval(value)
          rescue StandardError
          end
        end
        if key.is_a?(String)
          begin
            key = key.to_sym
          rescue StandardError
          end
        end
        [key, value]
      }]
    else
      testopts = {}
    end

    # -- run the command

    result = nil
    begin

      # - save variable if requested

      if testopts && testopts.key?(:variable)
        queryString = "#{testopts[:variable]} = (#{src})" # handle cases like: r(1) + 3
      else
        queryString = "(#{src})" # handle cases like: r(1) + 3
      end

      # - run the command

      result = $defines.eval(queryString)

      # - run as a query if it is one

      if result.kind_of?(RethinkDB::Cursor) || result.kind_of?(Enumerator)
        print_debug("Evaluating cursor: #{queryString} #{testopts}")

      elsif result.kind_of?(RethinkDB::RQL)

        if opthash and opthash.length > 0
          optstring = opthash.to_s[1..-2].gsub(/(?<quoteChar>['"]?)\b(?<!:)(?<key>\w+)\b\k<quoteChar>\s*=>/, ':\k<key>=>')
          queryString += '.run($reql_conn, ' + optstring + ')' # removing braces
        else
          queryString += '.run($reql_conn)'
        end
        print_debug("Running query: #{queryString} Options: #{testopts}")
        result = $defines.eval(queryString)
        print_debug("Result: #{result}")

        if result.kind_of?(RethinkDB::Cursor) # convert cursors into Enumerators to allow for poping single items
	      result = result.each
	      if testopts && testopts.key?(:variable)
		    $defines.eval("#{testopts[:variable]} = #{testopts[:variable]}.each")
		  end
	    end
      else
        print_debug("Running: #{queryString}")
      end

    rescue StandardError, SyntaxError => e
      result = e
    end

    if testopts && testopts.key?(:noreply_wait) && testopts[:noreply_wait]
        $reql_conn.noreply_wait
    end

    # -- process the result

    return check_result(name, src, result, expected, testopts)

  rescue StandardError, SyntaxError => err
    fail_test(name, src, err, expected, type="TEST")
  end
end

def setup_table(table_variable_name, table_name, db_name="test")

  if $required_external_tables.count > 0
    # use one of the required tables
    db_name, table_name = $required_external_tables.pop
    begin
        r.db(db_name).table(table_name).info().run($reql_conn)
    rescue RethinkDB::ReqlRuntimeError
      "External table #{db_name}.#{table_name} did not exist"
    end

    puts("Using existing table: #{db_name}.#{table_name}, will be: #{table_variable_name}")

    at_exit do
      res = r.db(db_name).table(table_name).delete().run($reql_conn)
      raise "Failed to clean out contents from table #{db_name}.#{table_name}: #{res}" unless res["errors"] == 0
      r.db(db_name).table(table_name).index_list().for_each{|row| r.db(db_name).table(table_name).index_drop(row)}.run($reql_conn)
    end
  else
    # create a new table
    if r.db(db_name).table_list().set_intersection([table_name]).count().eq(1).run($reql_conn)
      res = r.db(db_name).table_drop(table_name).run($reql_conn)
      raise "Unable to delete table before use #{db_name}.#{table_name}: #{res}" unless res['errors'] == 0
    end
    res = r.db(db_name).table_create(table_name).run($reql_conn)
    raise "Unable to create table #{db_name}.#{table_name}: #{res}" unless res["tables_created"] == 1
    r.db(db_name).table(table_name).wait().run($reql_conn)
    
    print_debug("Created table: #{db_name}.#{table_name}, will be: #{table_variable_name}")
    $stdout.flush

    at_exit do
      res = r.db(db_name).table_drop(table_name).run($reql_conn)
      raise "Failed to delete table #{db_name}.#{table_name}: #{res}" unless res["tables_dropped"] == 1
    end
  end

  $defines.eval("#{table_variable_name} = r.db('#{db_name}').table('#{table_name}')")
end

def setup_table_check()
    if $required_external_tables.count > 0
      raise "Unused external tables, that is probably not supported by this test: {$required_external_tables}"
    end
end

def check_no_table_specified
  if DB_AND_TABLE_NAME != "no_table_specified"
    abort "This test isn't meant to be run against a specific table"
  end
end

at_exit do
  puts "Ruby: #{$success_count} of #{$test_count} tests passed. #{$test_count - $success_count} tests failed."
end

def check_result(name, src, result, expected, testopts={})
  begin
    # - process expected
    begin
      if expected && expected != ''
        expected = $defines.eval(expected.to_s)
      else
        expected = NoError
      end
    rescue Exception => err
      fail_test(name, src, err, expected, type="COMPARE SETUP ERROR")
      return false
    end
    
    # - read out cursors
    if (result.kind_of?(RethinkDB::Cursor) || result.kind_of?(Enumerator)) && not(expected == NoError || testopts.has_key?('variable'))
      begin
        result = result.to_a
      rescue Exception => err
        result = err
      end
    end
    
    # - compare the result
    begin
      if cmp_test(expected, result, testopts) != 0
        fail_test(name, src, result, expected)
        return false
      end
    rescue Exception => err
      fail_test(name, src, err, expected, type="COMPARE ERROR")
      return false
    end
    
    # - declare victory
    $success_count += 1
    return true
    
  rescue StandardError, SyntaxError => err
    fail_test(name, src, err, expected, type="UNEXPECTED COMPARE ERROR")
    return false
  end
end

def fail_test(name, src, result, expected, type="TEST FAILURE")
  $stderr.puts "#{type}: #{name}"
  $stderr.puts "     SOURCE:     #{src}"
  $stderr.puts "     EXPECTED:   #{show expected}"
  $stderr.puts "     RESULT:     #{show result}"
  if result && result.respond_to?(:backtrace)
    $stderr.puts "     BACKTRACE:\n<<<<<<<<<\n#{result.backtrace.join("\n")}\n>>>>>>>>>"
  end
  if show(result) != result
    $stderr.puts "     RAW RESULT:\n<<<<<<<<<\n#{result}\n>>>>>>>>>"
  end
  $stderr.puts ""
end

def the_end
  if $test_count != $success_count then
    abort "Failed #{$test_count - $success_count} tests"
  end
end

def define(expr, variable=nil)
  if variable
    $defines.eval("#{variable} = #{expr}")
  else
    $defines.eval(expr)
  end
end

True=true
False=false
