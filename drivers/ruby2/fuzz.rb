#!/usr/bin/ruby
# Copyright 2010-2012 RethinkDB, all rights reserved.
load 'quickstart2.rb'

$s = ARGV[0] ? ARGV[0].to_i : srand()
srand($s)
$f = File.open("#{$$}.log", "w")
$f.puts $s
puts "#{$$} #{$s}"

include RethinkDB

$port_base = 32600
$port_base ||= ARGV[0].to_i # 0 if none given

$seed_terms ||= eval(`cat ./fuzz_seed.rb`).map{|x| x.to_pb}
$terms = $seed_terms
def expand_term term
  [term, term.args.map{|x| expand_term(x)}, term.optargs.map{|x| expand_term(x.val)}]
end
$seed_chunks = $seed_terms.map{|term| expand_term(term)}.flatten
$chunks = $seed_chunks

$i = 0

class Term2
  def shallow_dup
    t = Term2.new
    t.type = type
    t.datum = datum
    args.each {|x| t.args << x}
    optargs.each {|x| t.optargs << x}
    t
  end
end

class Fuzzer
  def initialize val
    raise SyntaxError, "ERR" if !val
    @val = val
  end
  def num_ops; ops.map{|k,v| k}.reduce(:+); end
  def random_op
    ind = rand(num_ops)
    ops.each {|op|
      ind -= op[0]
      return op[1..-1] if ind < 0
    }
    raise SyntaxError, "You can't add."
  end
  def exec_random_op
    op = random_op
    # PP.pp [caller.size, op]
    send(*op) if rand(caller.size) < 200
    @val
  end
  def multi
    exec_random_op
    exec_random_op
  end
  def do_nothing; end
end

class PacketFuzzer < Fuzzer
  @@ops = [[20, :flip_1_bit],
           [10, :flip_n_bits, 0.01],
           [10, :flip_n_bits, 0.1],
           [5, :flip_n_bits, 0.3],
           [5, :flip_n_bits, 0.5],
           [5, :flip_n_bits, 0.9],
           [40, :swap_1_byte],
           [20, :swap_n_bytes, 0.01],
           [20, :swap_n_bytes, 0.1],
           [10, :swap_n_bytes, 0.3],
           [10, :swap_n_bytes, 0.5],
           [10, :swap_n_bytes, 0.9],
           [100, :multi],
           [200, :do_nothing]]
  def ops; @@ops; end

  def random_index; rand(@val.size-4)+4; end
  def run_each_index(&b)
    (4...@val.size).to_a.each(&b)
  end
  def random_byte; 1 << rand(8); end

  def flip_1_bit
    @val[random_index] ^= random_byte
  end
  def flip_n_bits prob
    run_each_index {|i| @val[i] ^= random_byte if rand < prob }
  end

  def swap_1_byte(i = random_index, f = random_index)
    @val[i], @val[f] = @val[f], @val[i]
  end
  def swap_n_bytes prob
    run_each_index {|i| swap_1_byte(i) if rand < prob}
  end
end

class ProtobFuzzer < Fuzzer
  @@ops = [[50, :add_1],
           [50, :replace_1],
           [10, :replace_n, 0.1],
           [10, :replace_n, 0.3],
           [10, :replace_n, 0.5],
           [10, :replace_n, 0.9],
           [100, :recurse_1],
           [20, :recurse_n, 0.1],
           [20, :recurse_n, 0.3],
           [20, :recurse_n, 0.5],
           [20, :recurse_n, 0.9],
           [100, :multi],
           [100, :do_nothing]]
  def ops; @@ops; end

  def add_chunk term
    # $terms << term
    # $terms << $seed_terms[rand($seed_terms.size)]
    # $chunks << term
    # $chunks << $seed_chunks[rand($seed_chunks.size)]
    term
  end

  def exec_random_op
    return @val if @val.type == Term2::TermType::DATUM
    add_chunk super
  end

  def modify_1
    num_opts = @val.args.size + @val.optargs.size
    return if num_opts == 0
    i = rand num_opts
    if i < @val.args.size
      @val.args[i] = yield(i, @val.args[i])
    else
      i -= @val.args.size
      @val.optargs[i].val = yield(@val.optargs[i].key, @val.optargs[i].val)
    end
  end
  def modify_n
    @val.args.each_index {|i|
      @val.args[i] = yield(i, @val.args[i])
    }
    @val.optargs.each_index {|i|
      @val.optargs[i].val = yield(@val.optargs[i].key, @val.optargs[i].val)
    }
  end

  def rand_term; $chunks[rand($chunks.size)].dup; end

  def add_1
    @val.args << rand_term
  end
  def replace_1
    modify_1 {|k,v| rand_term}
  end
  def replace_n prob
    modify_n {|k,v| rand < prob ? rand_term : v }
  end
  def recurse_1
    modify_1 {|k,v| ProtobFuzzer.new(v).exec_random_op}
  end
  def recurse_n prob
    modify_n {|k,v| rand < prob ? ProtobFuzzer.new(v).exec_random_op : v}
  end
end

class FuzzConn < Connection
  def packet_fuzz packet
    x = PacketFuzzer.new(packet).exec_random_op
  end

  def protob_fuzz msg
    # puts "pb_fuzz start"
    term2 = ProtobFuzzer.new(msg.query).exec_random_op
    msg.query = term2
    begin
      $f.puts $i, RPP.pp(msg.query)
    rescue Exception => e
      # puts "unprintable"
    end
    # puts "pb_fuzz end"
    $msg = msg
    msg
  end

  def send packet
    # puts "sending #{packet.inspect}"
    super packet_fuzz(packet)
  end
  def dispatch msg
    super protob_fuzz(msg)
  end

  def wait(*a)
    raise RuntimeError, "The fuzzer should catch this."
    super
  end

  def run(*a)
    begin
      PP.pp super(*a)
      # raise SyntaxError, "unreachable"
    rescue RuntimeError => e
    end
    puts "ran #{$i}" if $i%100 == 0
    $i += 1;
  end
end

$c = FuzzConn.new('localhost', $port_base + 28015).repl

while true
  RQL.new($terms[rand($terms.size)]).run
end
