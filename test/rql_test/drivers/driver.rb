require 'pp'

$test_count = 0
$failure_count = 0
$success_count = 0

JSPORT = ARGV[0]
CPPPORT = ARGV[1]
DB_AND_TABLE_NAME = ARGV[2]

# -- import the called-for rethinkdb module
if ENV['RUBY_DRIVER_DIR']
  $LOAD_PATH.unshift ENV['RUBY_DRIVER_DIR']
  require 'rethinkdb'
  $LOAD_PATH.shift
else
  # look for the source directory
  targetPath = File.expand_path(File.dirname(__FILE__))
  while targetPath != File::Separator
    sourceDir = File.join(targetPath, 'drivers', 'ruby')
    if File.directory?(sourceDir)
      unless system("make -C " + sourceDir)
        abort "Unable to build the ruby driver at: " + sourceDir
      end
      $LOAD_PATH.unshift(File.join(sourceDir, 'lib'))
      require 'rethinkdb'
      $LOAD_PATH.shift
      break
    end
    targetPath = File.dirname(targetPath)
  end
end
extend RethinkDB::Shortcuts

def show x
  if x.class == Err
    name = x.type.sub(/^RethinkDB::/, "")
    return "<#{name} #{'~ ' if x.regex}#{show x.message}>"
  end
  return (PP.pp x, "").chomp
end

NoError = "nope"
AnyUUID = "<any uuid>"
Err = Struct.new(:type, :message, :backtrace, :regex)
Bag = Struct.new(:items)

def bag list
  Bag.new(list)
end

def arrlen len, x
  Array.new len, x
end

def uuid
  AnyUUID
end

def err(type, message, backtrace=[])
  Err.new(type, message, backtrace, false)
end

def err_regex(type, message, backtrace=[])
  Err.new(type, message, backtrace, true)
end

def eq_test(one, two, testopts={})
  return cmp_test(one, two, testopts) == 0
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

def cmp_test(one, two, testopts={})
  if two.object_id == NoError.object_id
    return -1 if one.class == Err
    return 0
  end
  
  if one.nil? and two.nil?
    return 0
  elsif one.nil?
    return 1
  elsif two.nil?
    return -1
  end
  
  if two.object_id == AnyUUID.object_id
    return -1 if not one.kind_of? String
    return 0 if one.match /[a-z0-9]{8}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{12}/
    return 1
  end

  if one.class == String then
    one = one.sub(/\nFailed assertion:(.|\n)*/, "")
  end

  case "#{two.class}"
  when "Err"
    if one.kind_of? Exception
      one = Err.new("#{one.class}".sub(/^RethinkDB::/,""), one.message, false)
    end
    cmp = one.class.name <=> two.class.name
    return cmp if cmp != 0
    if not two.regex
      one_msg = one.message.sub(/:\n(.|\n)*|:$/, ".")
      [one.type, one_msg] <=> [two.type, two.message]
    else
      if (Regexp.compile two.type) =~ one.type and
          (Regexp.compile two.message) =~ one.message
        return 0
      end
      return -1
    end

  when "Array"
    if one.respond_to? :to_a
      one = one.to_a
    end
    cmp = one.class.name <=> two.class.name
    return cmp if cmp != 0
    cmp = one.length <=> two.length
    return cmp if cmp != 0
    one.zip(two) { |pair|
      cmp = cmp_test(pair[0], pair[1], testopts)
      return cmp if cmp != 0
    }
    return 0

  when "Hash"
    cmp = one.class.name <=> two.class.name
    return cmp if cmp != 0
    one = Hash[ one.map{ |k,v| [k.to_s, v] } ]
    two = Hash[ two.map{ |k,v| [k.to_s, v] } ]
    cmp = one.keys.sort <=> two.keys.sort
    return cmp if cmp != 0
    one.each_key { |key|
      cmp = cmp_test(one[key], two[key], testopts)
      return cmp if cmp != 0
    }
    return 0

  when "Bag"
    return cmp_test(one.sort{ |a, b| cmp_test(a, b, testopts) },
                    two.items.sort{ |a, b| cmp_test(a, b, testopts) }, testopts)
  
  when "Float", "Fixnum", "Number"
    if not (one.kind_of? Float or one.kind_of? Fixnum)
      cmp = one.class.name <=> two.class.name
      return cmp if cmp != 0
    end
    
    if two.kind_of?(Number)
      if not one.kind_of?(two.requiredType)
        cmp = one.class.name <=> two.class.name
        return cmp if cmp != 0 else -1
      end
      two = two.value
    end
    
    if testopts.has_key?(:precision)
      diff = one - two
      if (diff).abs < testopts[:precision]
        return 0
      else
        return diff <=> 0
      end
    else
      return one <=> two
    end
  
  else
    begin
      cmp = one <=> two
      return cmp if cmp != nil
      return one.class.name <=> two.class.name
    rescue
      return one.class.name <=> two.class.name
    end
  end
end

def eval_env; binding; end
$defines = eval_env

$cpp_conn = RethinkDB::Connection.new(:host => 'localhost', :port => CPPPORT)
begin
  r.db_create('test').run($cpp_conn)
rescue
end

def test src, expected, name, opthash=nil, testopts=nil
  if opthash
    $opthash = Hash[opthash.map{|k,v| [k, eval(v, $defines)]}]
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
      queryString += '.run($cpp_conn, ' + opthash.to_s + ')'
    else
      queryString += '.run($cpp_conn)'
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

# Generated code must call either `setup_table` or `check_no_table_specified`
def setup_table table_variable_name, table_name
  at_exit do
    if DB_AND_TABLE_NAME == "no_table_specified"
      res = r.db("test").table_drop(table_name).run($cpp_conn)
      if res["dropped"] != 1
        abort "Could not drop table: #{res}"
      end
    else
      parts = DB_AND_TABLE_NAME.split('.')
      res = r.db(parts.first).table(parts.last).delete().run($cpp_conn)
      if res["errors"] != 0
        abort "Could not clear table: #{res}"
      end
      res = r.db(parts.first).table(parts.last).index_list().for_each{|row|
        r.db(parts.first).table(parts.last).index_drop(row)}.run($cpp_conn)
      if res.has_key?("errors") and res["errors"] != 0
        abort "Could not drop indexes: #{res}"
      end
    end
  end
  if DB_AND_TABLE_NAME == "no_table_specified"
    res = r.db("test").table_create(table_name).run($cpp_conn)
    if res["created"] != 1
      abort "Could not create table: #{res}"
    end
      $defines.eval("#{table_variable_name} = r.db('test').table('#{table_name}')")
    else
      parts = DB_AND_TABLE_NAME.split('.')
      $defines.eval("#{table_variable_name} = r.db(\"#{parts.first}\").table(\"#{parts.last}\")")
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

def check_result(name, src, res, expected, testopts={})
  sucessfulTest = true
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
    sucessfulTest = false
  end
  if sucessfulTest
    begin
      if ! eq_test(res, expected, testopts)
        fail_test name, src, res, expected
        sucessfulTest = false
      end
    rescue Exception => e
      sucessfulTest = false
      puts "#{name}: Error: #{e} when comparing #{show res} and #{show expected}"
    end
  end
  if sucessfulTest
    $success_count += 1
    return true
  else
    $failure_count += 1
    return false
  end
end

def fail_test name, src, res, expected
  $stderr.puts "TEST FAILURE: #{name}"
  $stderr.puts "\tBODY: #{src}"
  $stderr.puts "\tVALUE: #{show res}"
  $stderr.puts "\tEXPECTED: #{show expected}"
  $stderr.puts ""
end

def the_end
  if $failure_count != 0 then
    abort "Failed #{$failure_count} tests"
  end
end

def define expr
  eval expr, $defines
end

True=true
False=false

