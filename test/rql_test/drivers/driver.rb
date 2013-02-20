$: << '../../drivers/ruby2/lib'
$: << '../../build/drivers/ruby/rdb_protocol'
require 'pp'
require 'rethinkdb'
extend RethinkDB::Shortcuts

JSPORT = ARGV[0]
CPPPORT = ARGV[1]

def eq_test(one, two)

  case "#{two.class}"
  when "Array"
    return false if one.class != two.class
    return false if one.length != two.length
    return one.zip(two).map{ |a, b| eq_test(a, b) }.all?

  when "Hash"
    return false if one.class != two.class
    one = Hash[ one.map{ |k,v| [k.to_s, v] } ]
    two = Hash[ two.map{ |k,v| [k.to_s, v] } ]
    return false if one.keys.sort != two.keys.sort
    return one.keys.map{|k| eq_test(one[k], two[k])}.all?

  when "Bag"
    return eq_test(one.sort, two.items.sort)

  else
    if not [Fixnum, Float].member? one.class
      return false if one.class != two.class
    end
    return one == two
  end
end

def show x
  return (PP.pp x, "").chomp
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
  rescue Exception => e
    puts "#{name}: Error: #{e} running query #{src}"
    return false
    end
  
  begin
    expected = eval expected.to_s, $defines
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

Bag = Struct.new(:items)

def bag list
  Bag.new(list)
end

True=true
False=false

