$: << '../../drivers/ruby2/lib'
require 'rethinkdb'
extend RethinkDB::Shortcuts

JSPORT = ARGV[0]
CPPPORT = ARGV[1]

def eq_test(one, two)
  return false if one.class != two.class

  case one.class
  when Array
      return false if one.length != two.length
      return one.zip(two).map(eq_test).all?

  when Hash
    return false if a.keys.sort != b.keys.sort
    return a.keys.map{|k| eq_test(a[k], b[k])}.all?

  else
    return one == two
  end
end

def eq exp
  proc { |val|
    if eq_test(one, two)
      puts "Equality comparison failed"
      puts "Value: #{val}, Expected: #{exp}"
      return false
    else
      return true
    end
  }
end

$tests = []

def eval_env; binding; end
$defines = eval_env

$cpp_conn = RethinkDB::Connection.new('localhost', CPPPORT)

$js_conn = RethinkDB::Connection.new('localhost', JSPORT)


def run_test
  tests.each do |test|
    begin
      query = eval test.src, $defines
    rescue Exception => e
      puts "Error: #{e} in construction of #{test.src}"
      next
    end
    
    begin
      cppres = $cpp_conn.run(query)
    rescue Exception => e
      puts "Error on running of query on CPP server: #{e}"
      next
    end
    
    begin
      jsres = $js_conn.run(query)
    rescue Exception => e
      puts "Error on running of query on JS server: #{e}"
      next
    end
    
    exp_fun = eval test.expected, $defines
    
    if ! exp_fun.kind_of? Proc
      exp_fun = eq exp_fun
    end

    if ! exp_fun.call cppres
      puts " in CPP version of: #{src}"
    end
    if ! exp_fun jsres
      puts " in JS version of: #{src}"
    end
  end
end

def test src, expected
  $tests << Struct.new(:src, :expected).new(src, expected)
end

def define expr
  eval expr, $defines
end

def bag list
  bag = eval(list).sort
  proc do |other|
    eq_test(bag, other.sort)
  end
end
