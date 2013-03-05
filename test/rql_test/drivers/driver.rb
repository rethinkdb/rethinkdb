$LOAD_PATH.unshift '../../drivers/ruby2/lib'
$LOAD_PATH.unshift '../../build/drivers/ruby/rdb_protocol'
require 'pp'
require 'rethinkdb'
extend RethinkDB::Shortcuts

JSPORT = ARGV[0]
CPPPORT = ARGV[1]

def show x
  return (PP.pp x, "").chomp
end

NoError = "nope"
Err = Struct.new(:type, :message, :backtrace)
Bag = Struct.new(:items)

def bag list
  Bag.new(list)
end

def err(type, message, backtrace)
  Err.new(type, message, backtrace)
end

def eq_test(one, two)
  return cmp_test(one, two) == 0
end

def cmp_test(one, two)

  if two.object_id == NoError.object_id
    return -1 if one.class == Err
    return 0
  end

  case "#{two.class}"
  when "Err"
    cmp = one.class.name <=> two.class.name
    return cmp if cmp != 0
    [one.type, one.message] <=> [two.type, two.message]

  when "Array"
    cmp = one.class.name <=> two.class.name
    return cmp if cmp != 0
    cmp = one.length <=> two.length
    return cmp if cmp != 0
    return one.zip(two).reduce(0){ |acc, pair|
      acc == 0 ? cmp_test(pair[0], pair[1]) : acc
    }

  when "Hash"
    cmp = one.class.name <=> two.class.name
    return cmp if cmp != 0
    one = Hash[ one.map{ |k,v| [k.to_s, v] } ]
    two = Hash[ two.map{ |k,v| [k.to_s, v] } ]
    cmp = one.keys.sort <=> two.keys.sort
    return cmp if cmp != 0
    return one.keys.reduce(0){ |acc, k|
      acc == 0 ? cmp_test(one[k], two[k]) : acc
    }

  when "Bag"
    return cmp_test(one.sort{ |a, b| cmp_test a, b },
                    two.items.sort{ |a, b| cmp_test a, b })
    
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

$js_conn = RethinkDB::Connection.new('localhost', JSPORT)

$cpp_conn = RethinkDB::Connection.new('localhost', CPPPORT)

$test_count = 0
$success_count = 0

def test src, expected, name
  $test_count += 2
  begin
    query = eval src, $defines
  rescue Exception => e
    puts "#{name}: Error: '#{e}' in construction of #{src}"
    return
  end

  begin
    if do_test query, expected, $cpp_conn, name + '-CPP', src
      $success_count += 1
    end
    if do_test query, expected, $js_conn, name + '-JS', src
      $success_count += 1
    end
  rescue Exception => e
    puts "#{name}: Error: '#{e}' testing query #{src}"
  end
end

at_exit do
  puts "Ruby: #{$success_count} of #{$test_count} tests passed. #{$test_count - $success_count} tests failed."
end

def do_test query, expected, con, name, src
  begin
    res = query.run(con)
  rescue Exception => exc
    res = err("Rql" + exc.class.name, exc.message.split("\n")[0], "TODO")
  end
  
  begin
    if expected != ''
      expected = eval expected.to_s, $defines
    else
      expected = NoError
    end
    if ! eq_test(res, expected)
      puts "#{name}: Equality comparison failed for #{src}"
      puts "Value: #{show res}, Expected: #{show expected}"
      return false
    else
      return true
    end
  rescue Exception => e
    puts "#{name}: Error: #{e} when comparing #{show res} and #{show expected}"
    return false
  end
end

def define expr
  eval expr, $defines
end

True=true
False=false

