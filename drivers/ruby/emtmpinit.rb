load 'quickstart3.rb'

r.table_create('test').run rescue nil
r.table('test').delete.run
r.table('test').insert([{id: 0}, {id: 1}]).run

def assert(s = "")
  raise "Assertion failed: #{s}" if !yield
end
def assert_eq(a, b)
  assert("#{a.inspect} == #{b.inspect}"){a == b}
end

assert_eq(r.table('test').changes(include_states: true).limit(1).run.to_a,
          [{'state' => 'ready'}])
assert_eq(r.table('test').changes(include_initial_vals: true).limit(2).run.to_a,
          [{"new_val"=>{"id"=>0}}, {"new_val"=>{"id"=>1}}])
assert_eq(r.table('test').changes(include_initial_vals: true,
                                  include_states: true).limit(4).run.to_a,
          [{"state"=>"initializing"}, {"new_val"=>{"id"=>0}},
           {"new_val"=>{"id"=>1}}, {"state"=>"ready"}])
