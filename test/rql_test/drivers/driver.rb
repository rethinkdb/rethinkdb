require 'pp'
require 'set'

$test_count = 0
$failure_count = 0
$success_count = 0

DEBUG_ENABLED = ENV['VERBOSE'] ? ENV['VERBOSE'] == 'true' : false

def print_debug(message)
    if DEBUG_ENABLED
        puts('DEBUG: ' + message)
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

$defines = binding

# --

def show(x)
  if x.class == Err
    name = x.type.sub(/^RethinkDB::/, "")
    return "<#{name} #{'~ ' if x.regex}#{show x.message}>"
  end
  return (PP.pp x, "").chomp
end

NoError = "<no error>"
AnyUUID = "<any uuid>"
Err = Struct.new(:type, :message, :backtrace, :regex)
Bag = Struct.new(:items, :partial)
PartitalHash = Struct.new(:hash)

def bag(list, partial=false)
  Bag.new(list, partial)
end

def partial(expected)
  if expected.kind_of?(Array)
    bag(expected, true)
  elsif expected.kind_of?(Bag)
    bag(expected.items, true)
  elsif expected.kind_of?(Hash)
    PartitalHash.new(expected)
  else
    raise("partial can only handle Hashs, Arrays, or Bags. Got: #{expected.class}")
  end
end

def arrlen(len, x)
  Array.new(len, x)
end

def uuid
  AnyUUID
end

def shard
  # do nothing in Ruby tests
end

def err(type, message, backtrace=[])
  Err.new(type, message, backtrace, false)
end

def err_regex(type, message, backtrace=[])
  Err.new(type, message, backtrace, true)
end

def eq_test(expected, result, testopts={})
  return cmp_test(expected, result, testopts) == 0
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

def cmp_test(expected, result, testopts={}, partial=false)
  if expected.object_id == NoError.object_id
    if result.class == Err
      puts result
      puts result.backtrace
      return -1
    end
    return 0
  end
  
  if expected.nil? and result.nil?
    return 0
  elsif result.nil?
    return 1
  elsif expected.nil?
    return -1
  end
  
  if expected.object_id == AnyUUID.object_id
    return -1 if not result.kind_of? String
    return 0 if result.match /[a-z0-9]{8}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{12}/
    return 1
  end

  if result.class == String then
    result = result.sub(/\nFailed assertion:(.|\n)*/, "")
  end

  case "#{expected.class}"
  when "Err"
    if result.kind_of? Exception
      result = Err.new("#{result.class}".sub(/^RethinkDB::/,""), result.message, false)
    end
    cmp = result.class.name <=> expected.class.name
    return cmp if cmp != 0
    if not expected.regex
      result_msg = result.message.sub(/:\n(.|\n)*|:$/, ".")
      [result.type, result_msg] <=> [expected.type, expected.message]
    else
      if (Regexp.compile expected.type) =~ result.type and
          (Regexp.compile expected.message) =~ result.message
        return 0
      end
      return -1
    end

  when "Array"
    if result.respond_to? :to_a
      result = result.to_a
    end
    cmp = result.class.name <=> expected.class.name
    return cmp if cmp != 0
    if partial
      resultKeys = result.sort.each
      expected.sort.each { |expectedKey|
        cmp = -1
        for resultKey in resultKeys
          cmp = cmp_test(expectedKey, resultKey, testopts)
          if cmp == 0
            break
          end
        end
        if cmp != 0
          return cmp
        end
      }
    else
      cmp = result.length <=> expected.length
      return cmp if cmp != 0
      expected.zip(result) { |pair|
        cmp = cmp_test(pair[0], pair[1], testopts)
        return cmp if cmp != 0
      }
    end
    return 0
  
  when "PartitalHash"
    return cmp_test(expected.hash, result, testopts, true)
  
  when "Hash"
    cmp = result.class.name <=> expected.class.name
    return cmp if cmp != 0
    result = Hash[ result.map{ |k,v| [k.to_s, v] } ]
    expected = Hash[ expected.map{ |k,v| [k.to_s, v] } ]
    if partial
        if not Set.new(expected.keys).subset?(Set.new(result.keys))
          return -1
        end
    else
        cmp = result.keys.sort <=> expected.keys.sort
        return cmp if cmp != 0
    end
    expected.each_key { |key|
      cmp = cmp_test(expected[key], result[key], testopts)
      return cmp if cmp != 0
    }
    return 0

  when "Bag"
    return cmp_test(
      expected.items.sort{ |a, b| cmp_test(a, b, testopts) },
      result.sort{ |a, b| cmp_test(a, b, testopts) },
      testopts,
      expected.partial
    )
  
  when "Float", "Fixnum", "Number"
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
  if opthash
    $opthash = Hash[opthash.map{|k,v| [k, v.is_a?(String) ? eval(v, $defines) : v]}]
    if !$opthash[:max_batch_rows]
      $opthash[:max_batch_rows] = 3
    end
  else
    $opthash = {max_batch_rows: 3}
  end
  $test_count += 1
  
  if not (testopts and testopts.key?(:'reql-query') and testopts[:'reql-query'].to_s().downcase == 'false')
    # check that it evaluates without running it
    begin
      eval(src, $defines)
    rescue Exception => e
      result = err(e.class.name.sub(/^RethinkDB::/, ""), e.message.split("\n")[0], "TODO")
      return check_result(name, src, result, expected, testopts)
    end
  end
  
  # construct the query
  queryString = ''
  if testopts and testopts.key?(:'variable')
    queryString += testopts[:'variable'] + " = "
  end
  
  if not (testopts and testopts.key?(:'reql-query') and testopts[:'reql-query'].to_s().downcase == 'false')
    queryString += '(' + src + ')' # handle cases like: r(1) + 3
    if opthash
      opthash.each{ |key, value| opthash[key] = eval(value.to_s)}
      queryString += '.run($reql_conn, ' + opthash.to_s + ')'
    else
      queryString += '.run($reql_conn)'
    end
  else
    queryString += src
  end
  
  # run the query
  begin
    result = eval queryString, $defines
  rescue Exception => e
    result = err(e.class.name.sub(/^RethinkDB::/, ""), e.message.split("\n")[0], "TODO")
  end
  return check_result(name, src, result, expected, testopts)
  
end

def setup_table(table_variable_name, table_name, db_name="test")
  
  if $required_external_tables.count > 0
    # use one of the required tables
    table_name, db_name = $required_external_tables.pop
    raise "External table #{db_name}.#{table_name} did not exist" unless r.db(db_name).table_list().set_intersection([table_name]).count().eq(1).run($reql_conn)
    
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
    
    print_debug("Created table: #{db_name}.#{table_name}, will be: #{table_variable_name}")
    $stdout.flush
    
    at_exit do
      res = r.db(db_name).table_drop(table_name).run($reql_conn)
      raise "Failed to delete table #{db_name}.#{table_name}: #{res}" unless res["tables_dropped"] == 1
    end
  end
  
  $defines.eval("#{table_variable_name} = r.db('#{db_name}').table('#{table_name}')")
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
  successfulTest = true
  begin
    if expected && expected != ''
      expected = eval expected.to_s, $defines
    else
      expected = NoError
    end
  rescue Exception => e
    $stderr.puts "SETUP ERROR: #{name}"
    $stderr.puts "\tBODY: #{src}"
    $stderr.puts "\tEXPECTED: #{show expected}"
    $stderr.puts "\tFAILURE: #{e}"
    $stderr.puts ""
    successfulTest = false
  end
  if successfulTest
    begin
      if ! eq_test(expected, result, testopts)
        fail_test(name, src, result, expected)
        successfulTest = false
      end
    rescue Exception => e
      successfulTest = false
      puts "#{name}: Error: #{e} when comparing #{show result} and #{show expected}"
    end
  end
  if successfulTest
    $success_count += 1
    return true
  else
    $failure_count += 1
    return false
  end
end

def fail_test(name, src, result, expected, type="TEST")
  $stderr.puts "TEST FAILURE: #{name}"
  $stderr.puts "\tBODY: #{src}"
  $stderr.puts "\tVALUE: #{show result}"
  $stderr.puts "\tEXPECTED: #{show expected}"
  if result && result.kind_of?(Exception)
    $stderr.puts "\tEXCEPTION: #{result.message}"
    $stderr.puts result.backtrace.join("\n")
  end
  $stderr.puts ""
end

def the_end
  if $failure_count != 0 then
    abort "Failed #{$failure_count} tests"
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

# REPLACE WITH TABLE CREATION LINES

if $required_external_tables.count > 0
  raise "Unused external tables, that is probably not supported by this test: {$required_external_tables}"
end