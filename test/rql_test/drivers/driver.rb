require 'pp'

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
      if system("make -C " + sourceDir) != 0
        abort "Unable to build the ruby driver at: " + sourceDir
      end
      $LOAD_PATH.unshift = File.join(sourceDir, 'lib');
      require 'rethinkdb'
      $LOAD_PATH.shift
      break
    end
    targetPath = File.dirname(targetPath)
  end
end
extend RethinkDB::Shortcuts

JSPORT = ARGV[0]
CPPPORT = ARGV[1]

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
Int = Struct.new(:i)
Floatable = Struct.new(:i)

def bag list
  Bag.new(list)
end

def arrlen len, x
  Array.new len, x
end

def uuid
  AnyUUID
end

def err(type, message, backtrace)
  Err.new(type, message, backtrace, false)
end

def err_regex(type, message, backtrace=[])
  Err.new(type, message, backtrace, true)
end

def eq_test(one, two)
  return cmp_test(one, two) == 0
end

def int_cmp i
    Int.new i
end

def float_cmp i
    Floatable.new i
end

def cmp_test(one, two)

  if two.object_id == NoError.object_id
    return -1 if one.class == Err
    return 0
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

  when "Int"
    return cmp_test([Fixnum.name, two.i], [one.class.name, one])

  when "Floatable"
    return cmp_test([Float, two.i], [one.class, one])

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

# $js_conn = RethinkDB::Connection.new('localhost', JSPORT)

$cpp_conn = RethinkDB::Connection.new(:host => 'localhost', :port => CPPPORT)
begin
  r.db_create('test').run($cpp_conn)
rescue
end

$test_count = 0
$success_count = 0

def test src, expected, name, opthash=nil
  if opthash
    $opthash = Hash[opthash.map{|k,v| [k, eval(v, $defines)]}]
    if !$opthash[:batch_conf]
        $opthash[:batch_conf] = {max_els: 3}
    end
  else
    $opthash = {batch_conf: {max_els: 3}}
  end
  $test_count += 1
  begin
    query = eval src, $defines
  rescue Exception => e
    do_res_test name, src, e, expected
    return
  end

  begin
    do_test query, expected, $cpp_conn, name + '-CPP', src
    # do_test query, expected, $js_conn, name + '-JS', src
  rescue Exception => e
    do_res_test name, src, e, expected
  end
end

at_exit do
  puts "Ruby: #{$success_count} of #{$test_count} tests passed. #{$test_count - $success_count} tests failed."
end

def do_test query, expected, con, name, src
  begin
    if $opthash
      res = query.run(con, $opthash)
    else
      res = query.run(con)
    end
  rescue Exception => exc
    res = err(exc.class.name.sub(/^RethinkDB::/, ""), exc.message.split("\n")[0], "TODO")
  end
  return do_res_test name, src, res, expected
end

def do_res_test name, src, res, expected
  begin
    if expected != ''
      expected = eval expected.to_s, $defines
    else
      expected = NoError
    end
    if ! eq_test(res, expected)
      fail_test name, src, res, expected
      return false
    else
      $success_count += 1
      return true
    end
  rescue Exception => e
    puts "#{name}: Error: #{e} when comparing #{show res} and #{show expected}"
    return false
  end
end

@failure_count = 0
def fail_test name, src, res, expected
  @failure_count = @failure_count + 1
  puts "TEST FAILURE: #{name}"
  puts "TEST BODY: #{src}"
  puts "\tVALUE: #{show res}"
  puts "\tEXPECTED: #{show expected}"
  puts; puts;
end

def the_end
  if @failure_count != 0 then
    abort "Failed #{@failure_count} tests"
  end
end

def define expr
  eval expr, $defines
end

True=true
False=false

