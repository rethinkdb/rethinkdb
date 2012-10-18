$LOAD_PATH.unshift('../rethinkdb')
require 'rethinkdb.rb'

class Transformation
  attr_accessor :priority, :block
  def initialize(priority, &block); @priority = priority; @block = block; end
  def ms(sexp); sexp; end
  def mangle_sexp(sexp)
    a = sexp.dup
    res = ms(a)
    return res.class == Array ? res : a
  end

  def mb(str); str; end
  def mangle_bytes(str)
    s = str.dup
    res = s == "" ? s : mb(s)
    return res.class == String ? res : s
  end
end

class Prob_Transformation < Transformation
  def initialize(priority, prob, &block); @prob = prob; super(priority, &block); end
end

################################################################################
##### Byte Transformations
################################################################################
class One_Byte_Transformation < Transformation
  def mb(s); block.call(s, rand(s.length)); end
end

class Multi_Byte_Transformation < Prob_Transformation
  def mb(s); s.chars.each_with_index{|_,i| block.call(s, i) if rand < @prob}; end
end

def mangle_byte(s, i); s[i] ^= rand(256); end
def zero_byte(s, i); s[i] = 0; end
def swap_byte(s, i) f = rand(s.size); s[i], s[f] = s[f], s[i]; end
################################################################################
##### Sexp Transformations
################################################################################

class Simple_Sexp_Op < Transformation
  def ms(sexp); block.call(sexp); end
end

class Random_Sexp_Op < Prob_Transformation
  def ms(sexp); block.call(sexp, @prob); end
end

def rand_sexp; $sexps[rand($sexps.size)]; end
class Random_Sexp_Pairing < Prob_Transformation
  def ms(sexp); block.call(sexp, rand_sexp, @prob); end
end

def crossover(s1, s2, prob, ret_immediately=false)
  return if s1.class != Array
  s1.each_with_index{|lhs, i|
    if rand < prob
      rhs = s2[(rand < prob || i >= s2.size) ? rand(s2.size) : i]
      if rhs.class != Array || rand < prob
        s1[i] = rhs
      else
        crossover(lhs.class == Array ? lhs: s1, rhs, ret_immediately)
      end
      return if ret_immediately
    end
  }
end
def single_crossover(s1, s2, prob); crossover(s1, s2, prob, true); end

def code_walk(sexp, visit_proc=nil, &sub_proc)
  visit_proc.call(sexp) if visit_proc
  (sexp.class == Array ? sexp : []).each_with_index {|s, i|
    code_walk(s, visit_proc, &sub_proc)
    sexp[i] = sub_proc[s, i];
  }
end
def code_walk_text(sexp)
  code_walk(sexp) {|x,i|
    if    x.class == String then yield(x,i)
    elsif x.class == Symbol then yield(x.to_s,i).to_sym
    else  x
    end
  }
end
def code_walk_numeric(sexp)
  code_walk(sexp) {|x,i| (x.kind_of? Numeric) ? yield(x,i) : x}
end

def var_collapse_all(sexp)
  code_walk_text(sexp){|x,_| x =~ /_var_/ ? '_var_' : x}
end

def var_collapse_positional(sexp)
  code_walk(sexp){|x,i| x =~ /_var_/ ? "_var_#{i}" : x}
end

def var_collapse_rand_3(sexp)
  code_walk(sexp){|x,_| x =~ /_var_/ ? "_var_#{rand(3)}" : x}
end

$numeric_edge_cases = {
  Fixnum => [0, 1, -1],
  Bignum => [2**62, 2**100],
  Float =>  [0.0, -1.0, 1.0, -0.1, 0.1, 2.0**1023, -2.0**1023]
}
def num_edge_cases(sexp, prob)
  code_walk_numeric(sexp) {|x,_|
    return x if not rand < prob
    return rand(100) if rand < prob**3
    target_class = rand < prob ? [Fixnum, Bignum, Float].choice : x.class
    $numeric_edge_cases[target_class].choice
  }
end

################################################################################
##### External Interface
################################################################################
class Transformer
  def initialize(conf)
    @transformations = []
    eval(`cat #{conf}`).each {|cls,args|
      @transformations += args.map{|x| cls.new(*x[0...-1], &method(x[-1]))}
    }
    @total_priority = @transformations.map{|x| x.priority}.reduce(:+)
  end

  def get_transformation
    target = rand(@total_priority)
    @transformations.each{|t| return t if (target -= t.priority) < 0}
    raise RuntimeError,"Unreachable."
  end

  def continue_with_sexp(new_sexp, sexp)
    return new_sexp != sexp || rand < 0.1
  end

  def continue_with_payload(new_payload, payload)
    return new_payload != payload || rand < 0.1
  end

  def [](sexp, retries=50)
    payload = nil
    (0...retries).each {|i|
      tr = get_transformation
      begin
        new_sexp = tr.mangle_sexp(sexp)
        if continue_with_sexp(new_sexp, sexp)
          q = RethinkDB::RQL_Query.new(new_sexp).query
          payload = q.serialize_to_string
          break
        end
      rescue Exception => e
      end
    }
    return nil if not payload

    final_payload = nil
    (0...retries).each {|i|
      tr = get_transformation
      begin
        new_payload = tr.mangle_bytes(payload)
        if continue_with_payload(new_payload, payload)
          final_payload = new_payload
          break
        end
      rescue Exception => e
      end
    }
    return final_payload
  end
end
