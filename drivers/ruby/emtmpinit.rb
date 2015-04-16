load 'quickstart3.rb'

r.table_create('test').run rescue nil
r.table('test').delete.run
r.table('test').insert([{id: 0}]).run

def assert(s = "")
  raise "Assertion failed: #{s}" if !yield
end
def assert_eq(a, b)
  assert("#{a.inspect} == #{b.inspect}"){a == b}
end

def q(query, val)
  assert_eq(query.limit(val.size).run.to_a, val)
end

tbl = r.table('test')
q(tbl.changes(include_states: true),
  [{'state' => 'ready'}])
q(tbl.changes(include_initial_vals: false, include_states: true),
  [{'state' => 'ready'}])
q(tbl.changes(include_initial_vals: true, include_states: true),
  [{"state"=>"initializing"}, {"new_val"=>{"id"=>0}}, {"state"=>"ready"}])
q(tbl.changes(include_initial_vals: true),
  [{"new_val"=>{"id"=>0}}])
q(tbl.changes(include_initial_vals: true, include_states: false),
  [{"new_val"=>{"id"=>0}}])

btw = r.table('test').between(0, 1)
q(btw.changes,
  [{"new_val"=>{"id"=>0}}])
q(btw.changes(include_states: false),
  [{"new_val"=>{"id"=>0}}])
q(btw.changes(include_states: true),
  [{"state"=>"initializing"}, {"new_val"=>{"id"=>0}}, {"state"=>"ready"}])
q(btw.changes(include_initial_vals: true),
  [{"new_val"=>{"id"=>0}}])
q(btw.changes(include_initial_vals: true, include_states: false),
  [{"new_val"=>{"id"=>0}}])
q(btw.changes(include_initial_vals: true, include_states: true),
  [{"state"=>"initializing"}, {"new_val"=>{"id"=>0}}, {"state"=>"ready"}])
q(btw.changes(include_initial_vals: false, include_states: true),
  [{"state"=>"ready"}])
