require_relative './importRethinkDB.rb'

$port ||= (ARGV[0] || ENV['RDB_DRIVER_PORT'] || raise('driver port not supplied')).to_i
ARGV.clear
$c = r.connect(port: $port).repl

def assert(s = "")
  raise "Assertion failed: #{s}" if !yield
end
def assert_eq(a, b)
  assert("#{a.inspect} == #{b.inspect}"){a == b}
end

def q(query, val)
  PP.pp query
  assert_eq(query.limit(val.size).run.to_a, val)
end

r.table_create('test').run rescue nil
tbl = r.table('test')
tbl.delete.run
tbl.insert({id: 0}).run
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

ga = r.table('test').get_all(0)
q(ga.changes,
  [{"new_val"=>{"id"=>0}}])
q(ga.changes(include_states: false),
  [{"new_val"=>{"id"=>0}}])
q(ga.changes(include_states: true),
  [{"state"=>"initializing"}, {"new_val"=>{"id"=>0}}, {"state"=>"ready"}])
q(ga.changes(include_initial_vals: true),
  [{"new_val"=>{"id"=>0}}])
q(ga.changes(include_initial_vals: true, include_states: false),
  [{"new_val"=>{"id"=>0}}])
q(ga.changes(include_initial_vals: true, include_states: true),
  [{"state"=>"initializing"}, {"new_val"=>{"id"=>0}}, {"state"=>"ready"}])
q(ga.changes(include_initial_vals: false, include_states: true),
  [{"state"=>"ready"}])

get = r.table('test').get(0)
q(get.changes,
  [{"new_val"=>{"id"=>0}}])
q(get.changes(include_states: false),
  [{"new_val"=>{"id"=>0}}])
q(get.changes(include_states: true),
  [{"state"=>"initializing"}, {"new_val"=>{"id"=>0}}, {"state"=>"ready"}])
q(get.changes(include_initial_vals: true),
  [{"new_val"=>{"id"=>0}}])
q(get.changes(include_initial_vals: true, include_states: false),
  [{"new_val"=>{"id"=>0}}])
q(get.changes(include_initial_vals: true, include_states: true),
  [{"state"=>"initializing"}, {"new_val"=>{"id"=>0}}, {"state"=>"ready"}])
q(get.changes(include_initial_vals: false, include_states: true),
  [{"state"=>"ready"}])

lm = r.table('test').orderby(index: 'id').limit(1)
q(lm.changes,
  [{"new_val"=>{"id"=>0}}])
q(lm.changes(include_states: false),
  [{"new_val"=>{"id"=>0}}])
q(lm.changes(include_states: true),
  [{"state"=>"initializing"}, {"new_val"=>{"id"=>0}}, {"state"=>"ready"}])
q(lm.changes(include_initial_vals: true),
  [{"new_val"=>{"id"=>0}}])
q(lm.changes(include_initial_vals: true, include_states: false),
  [{"new_val"=>{"id"=>0}}])
q(lm.changes(include_initial_vals: true, include_states: true),
  [{"state"=>"initializing"}, {"new_val"=>{"id"=>0}}, {"state"=>"ready"}])
q(lm.changes(include_initial_vals: false, include_states: true),
  [{"state"=>"ready"}])

union = r.table('test') \
         .get_all(0) \
         .union(r.table('test').get_all(0)) \
         .union(r.table('test').between(0, 1)) \
         .union(r.table('test'))
q(union.changes,
  [{"new_val"=>{"id"=>0}}, {"new_val"=>{"id"=>0}}, {"new_val"=>{"id"=>0}}])
q(union.changes(include_initial_vals: true),
  [{"new_val"=>{"id"=>0}}, {"new_val"=>{"id"=>0}},
   {"new_val"=>{"id"=>0}}, {"new_val"=>{"id"=>0}}])
q(union.changes(include_states: false),
  [{"new_val"=>{"id"=>0}}, {"new_val"=>{"id"=>0}}, {"new_val"=>{"id"=>0}}])
q(union.changes(include_states: false, include_initial_vals: true),
  [{"new_val"=>{"id"=>0}}, {"new_val"=>{"id"=>0}},
   {"new_val"=>{"id"=>0}}, {"new_val"=>{"id"=>0}}])
q(union.changes(include_states: true),
  [{"state"=>"initializing"}, {"new_val"=>{"id"=>0}}, {"new_val"=>{"id"=>0}},
   {"new_val"=>{"id"=>0}}, {"state"=>"ready"}])
q(union.changes(include_states: true, include_initial_vals: false),
  [{"state"=>"ready"}])
q(union.changes(include_states: true, include_initial_vals: true),
  [{"state"=>"initializing"}, {"new_val"=>{"id"=>0}}, {"new_val"=>{"id"=>0}},
   {"new_val"=>{"id"=>0}}, {"new_val"=>{"id"=>0}}, {"state"=>"ready"}])

# Artificial
tbl = r.db('rethinkdb').table('_debug_scratch')
tbl.delete.run
tbl.insert({id: 0}).run
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

ga = r.table('test').get_all(0)
q(ga.changes,
  [{"new_val"=>{"id"=>0}}])
q(ga.changes(include_states: false),
  [{"new_val"=>{"id"=>0}}])
q(ga.changes(include_states: true),
  [{"state"=>"initializing"}, {"new_val"=>{"id"=>0}}, {"state"=>"ready"}])
q(ga.changes(include_initial_vals: true),
  [{"new_val"=>{"id"=>0}}])
q(ga.changes(include_initial_vals: true, include_states: false),
  [{"new_val"=>{"id"=>0}}])
q(ga.changes(include_initial_vals: true, include_states: true),
  [{"state"=>"initializing"}, {"new_val"=>{"id"=>0}}, {"state"=>"ready"}])
q(ga.changes(include_initial_vals: false, include_states: true),
  [{"state"=>"ready"}])

get = r.table('test').get(0)
q(get.changes,
  [{"new_val"=>{"id"=>0}}])
q(get.changes(include_states: false),
  [{"new_val"=>{"id"=>0}}])
q(get.changes(include_states: true),
  [{"state"=>"initializing"}, {"new_val"=>{"id"=>0}}, {"state"=>"ready"}])
q(get.changes(include_initial_vals: true),
  [{"new_val"=>{"id"=>0}}])
q(get.changes(include_initial_vals: true, include_states: false),
  [{"new_val"=>{"id"=>0}}])
q(get.changes(include_initial_vals: true, include_states: true),
  [{"state"=>"initializing"}, {"new_val"=>{"id"=>0}}, {"state"=>"ready"}])
q(get.changes(include_initial_vals: false, include_states: true),
  [{"state"=>"ready"}])

lm = r.table('test').orderby(index: 'id').limit(1)
q(lm.changes,
  [{"new_val"=>{"id"=>0}}])
q(lm.changes(include_states: false),
  [{"new_val"=>{"id"=>0}}])
q(lm.changes(include_states: true),
  [{"state"=>"initializing"}, {"new_val"=>{"id"=>0}}, {"state"=>"ready"}])
q(lm.changes(include_initial_vals: true),
  [{"new_val"=>{"id"=>0}}])
q(lm.changes(include_initial_vals: true, include_states: false),
  [{"new_val"=>{"id"=>0}}])
q(lm.changes(include_initial_vals: true, include_states: true),
  [{"state"=>"initializing"}, {"new_val"=>{"id"=>0}}, {"state"=>"ready"}])
q(lm.changes(include_initial_vals: false, include_states: true),
  [{"state"=>"ready"}])

union = r.table('test') \
         .get_all(0) \
         .union(r.table('test').get_all(0)) \
         .union(r.table('test').between(0, 1)) \
         .union(r.table('test'))
q(union.changes,
  [{"new_val"=>{"id"=>0}}, {"new_val"=>{"id"=>0}}, {"new_val"=>{"id"=>0}}])
q(union.changes(include_initial_vals: true),
  [{"new_val"=>{"id"=>0}}, {"new_val"=>{"id"=>0}},
   {"new_val"=>{"id"=>0}}, {"new_val"=>{"id"=>0}}])
q(union.changes(include_states: false),
  [{"new_val"=>{"id"=>0}}, {"new_val"=>{"id"=>0}}, {"new_val"=>{"id"=>0}}])
q(union.changes(include_states: false, include_initial_vals: true),
  [{"new_val"=>{"id"=>0}}, {"new_val"=>{"id"=>0}},
   {"new_val"=>{"id"=>0}}, {"new_val"=>{"id"=>0}}])
q(union.changes(include_states: true),
  [{"state"=>"initializing"}, {"new_val"=>{"id"=>0}}, {"new_val"=>{"id"=>0}},
   {"new_val"=>{"id"=>0}}, {"state"=>"ready"}])
q(union.changes(include_states: true, include_initial_vals: false),
  [{"state"=>"ready"}])
q(union.changes(include_states: true, include_initial_vals: true),
  [{"state"=>"initializing"}, {"new_val"=>{"id"=>0}}, {"new_val"=>{"id"=>0}},
   {"new_val"=>{"id"=>0}}, {"new_val"=>{"id"=>0}}, {"state"=>"ready"}])
