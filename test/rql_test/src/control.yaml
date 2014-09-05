desc: Tests RQL control flow structures
table_variable_name: tbl
tests:

    # Setup a second table
    - cd: r.db('test').table_create('test2')
      ot: ({'created':1})
      def: tbl2 = r.db('test').table('test2')

    ## FunCall

    - py: "r.expr(1).do(lambda v: v * 2)"
      js: r.expr(1).do(function(v) { return v.mul(2); })
      rb: r.expr(1).do{|v| v * 2 }
      ot: 2

    - py: "r.expr([0, 1, 2]).do(lambda v: v.append(3))"
      js: r([0, 1, 2]).do(function(v) { return v.append(3); })
      rb: r([0, 1, 2]).do{ |v| v.append(3) }
      ot: [0, 1, 2, 3]

    - py: "r.do(1, 2, lambda x, y: x + y)"
      js: r.do(1, 2, function(x, y) { return x.add(y); })
      rb: r.do(1, 2) {|x, y| x + y}
      ot: 3

    - py: "r.do(lambda: 1)"
      js: r.do(function() { return 1; })
      rb: r.do{1}
      ot: 1

    # do error cases
    - py: "r.do(1, 2, lambda x: x)"
      js: r.do(1, 2, function(x) { return x; })
      rb: r.do(1, 2) {|x| x}
      ot: err("RqlRuntimeError", 'Expected 1 argument but found 2.', [1])

    - py: "r.do(1, 2, 3, lambda x, y: x + y)"
      js: r.do(1, 2, 3, function(x, y) { return x.add(y); })
      rb: r.do(1, 2, 3) {|x, y| x + y}
      ot: err("RqlRuntimeError", 'Expected 2 arguments but found 3.', [1])

    - cd: "r.do(1)"
      ot: 1

    - js: r.do(1, function(x) {})
      cd: []
      ot: err("RqlDriverError", 'Anonymous function returned `undefined`. Did you forget a `return`?', [1])

    - js: r.do(1, function(x) { return undefined; })
      cd: []
      ot: err("RqlDriverError", 'Anonymous function returned `undefined`. Did you forget a `return`?', [1])

    - cd: r.do()
      py: [] # Case handled by native python error
      ot: err("RqlDriverError", 'Expected 1 or more arguments but found 0.', [1])


    # FunCall errors

    - py: "r.expr('abc').do(lambda v: v.append(3))"
      js: r('abc').do(function(v) { return v.append(3); })
      rb: r('abc').do{ |v| v.append(3) }
      ot: err("RqlRuntimeError", "Expected type ARRAY but found STRING.", [1, 0])

    - py: "r.expr('abc').do(lambda v: v + 3)"
      js: r('abc').do(function(v) { return v.add(3); })
      rb: r('abc').do{ |v| v + 3 }
      ot: err("RqlRuntimeError", "Expected type STRING but found NUMBER.", [1, 1])

    - py: "r.expr('abc').do(lambda v: v + 'def') + 3"
      js: r('abc').do(function(v) { return v.add('def'); }).add(3)
      rb: r('abc').do{ |v| v + 'def' } + 3
      ot: err("RqlRuntimeError", "Expected type STRING but found NUMBER.", [1])

    - py: "r.expr(0).do(lambda a,b: a + b)"
      js: r(0).do(function(a,b) { return a.add(b); })
      rb: r(0).do{ |a, b| a + b }
      ot: err("RqlRuntimeError", 'Expected 2 arguments but found 1.', [1])

    - py: "r.do(1, 2, lambda a: a)"
      js: r.do(1,2, function(a) { return a; })
      rb: r.do(1, 2) { |a| a }
      ot: err("RqlRuntimeError", 'Expected 1 argument but found 2.', [1])

    - cd: r.expr(5).do(r.row)
      rb: r(5).do{ |row| row }
      ot: 5

    ## Branch

    - cd: r.branch(True, 1, 2)
      ot: 1
    - cd: r.branch(False, 1, 2)
      ot: 2
    - cd: r.branch(1, 'c', False)
      ot: ("c")
    - cd: r.branch(null, {}, [])
      ot: ([])

    - cd: r.branch(r.db('test'), 1, 2)
      ot: err("RqlRuntimeError", "Expected type DATUM but found DATABASE.", [])
    - cd: r.branch(tbl, 1, 2)
      ot: err("RqlRuntimeError", "Expected type DATUM but found TABLE.", [])
    - cd: r.branch(r.error("a"), 1, 2)
      ot: err("RqlRuntimeError", "a", [])

    - cd: r.branch([], 1, 2)
      ot: 1
    - cd: r.branch({}, 1, 2)
      ot: 1
    - cd: r.branch("a", 1, 2)
      ot: 1
    - cd: r.branch(1.2, 1, 2)
      ot: 1

    # r.error()
    - cd: r.error('Hello World')
      ot: err("RqlRuntimeError", "Hello World", [0])

    - cd: r.error(5) # we might want to allow this eventually
      ot: err("RqlRuntimeError", "Expected type STRING but found NUMBER.", [0])

    # For kicks, let's test arity checking in map and filter (Python driver doesn't require this)
    - js: r.expr([1, 2, 3]).filter()
      cd: []
      ot: err("RqlDriverError", "Expected 1 argument (not including options) but found 0.", [0])
    - js: r.expr([1, 2, 3]).filter(1, 2)
      cd: []
      ot: err("RqlDriverError", "Expected 1 argument (not including options) but found 2.", [0])

    # r.js()
    - cd: r.js('1 + 1')
      ot: 2

    - cd: r.js('1 + 1; 2 + 2')
      ot: 4

    - cd: r.do(1, 2, r.js('(function(a, b) { return a + b; })'))
      ot: 3

    - cd: r.expr(1).do(r.js('(function(x) { return x + 1; })'))
      ot: 2

    - cd: r.expr('foo').do(r.js('(function(x) { return x + "bar"; })'))
      ot: "'foobar'"

    # js timeout optarg shouldn't be triggered
    - py: r.js('1 + 2', timeout=1.2)
      js: r.js('1 + 2', {timeout:1.2})
      ot: 3

    # js error cases
    - cd: r.js('(function() { return 1; })')
      ot: err("RqlRuntimeError", "Query result must be of type DATUM, GROUPED_DATA, or STREAM (got FUNCTION).", [0])

    - cd: r.js('function() { return 1; }')
      ot: 'err("RqlRuntimeError", "SyntaxError: Unexpected token (", [0])'

    # Play with the number of arguments in the JS function
    - cd: r.do(1, 2, r.js('(function(a) { return a; })'))
      ot: 1

    - cd: r.do(1, 2, r.js('(function(a, b, c) { return a; })'))
      ot: 1

    - cd: r.do(1, 2, r.js('(function(a, b, c) { return c; })'))
      ot: err("RqlRuntimeError", "Cannot convert javascript `undefined` to ql::datum_t.", [0])

    - cd: r.expr([1, 2, 3]).filter(r.js('(function(a) { return a >= 2; })'))
      ot: ([2, 3])

    - cd: r.expr([1, 2, 3]).map(r.js('(function(a) { return a + 1; })'))
      ot: ([2, 3, 4])

    - cd: r.expr([1, 2, 3]).map(r.js('1'))
      ot: err("RqlRuntimeError", "Expected type FUNCTION but found DATUM.", [0])

    - cd: r.expr([1, 2, 3]).filter(r.js('(function(a) {})'))
      ot: err("RqlRuntimeError", "Cannot convert javascript `undefined` to ql::datum_t.", [0])

    # What happens if we pass static values to things that expect functions
    - cd: r.expr([1, 2, 3]).map(1)
      ot: err("RqlRuntimeError", "Expected type FUNCTION but found DATUM.", [0])

    - cd: r.expr([1, 2, 3]).filter('foo')
      ot: ([1, 2, 3])
    - cd: r.expr([1, 2, 4]).filter([])
      ot: ([1, 2, 4])
    - cd: r.expr([1, 2, 3]).filter(null)
      ot: ([])

    - py: r.expr([1, 2, 4]).filter(False)
      js: r.expr([1, 2, 4]).filter(false)
      rb: r([1, 2, 4]).filter(false)
      ot: ([])

    # forEach
    - cd: tbl.count()
      ot: 0

    # Insert three elements
    - js: r([1, 2, 3]).forEach(function (row) { return tbl.insert({ id:row }) })
      py: r.expr([1, 2, 3]).for_each(lambda row:tbl.insert({ 'id':row }))
      rb: r([1, 2, 3]).for_each{ |row| tbl.insert({ :id => row }) }
      ot: ({'deleted':0.0,'replaced':0.0,'unchanged':0.0,'errors':0.0,'skipped':0.0,'inserted':3})

    - cd: tbl.count()
      ot: 3

    # Update each row to add additional attribute
    - js: r([1, 2, 3]).forEach(function (row) { return tbl.update({ foo:row }) })
      py: r.expr([1,2,3]).for_each(lambda row:tbl.update({'foo':row}))
      rb: r.expr([1,2,3]).for_each{ |row| tbl.update({ :foo => row }) }
      ot: ({'deleted':0.0,'replaced':9,'unchanged':0.0,'errors':0.0,'skipped':0.0,'inserted':0.0})

    # Insert three more elements (and error on three)
    - js: r([1, 2, 3]).forEach(function (row) { return [tbl.insert({ id:row }), tbl.insert({ id:row.mul(10) })] })
      py: r.expr([1,2,3]).for_each(lambda row:[tbl.insert({ 'id':row }), tbl.insert({ 'id':row*10 })])
      rb: r.expr([1,2,3]).for_each{ |row| [tbl.insert({ :id => row}), tbl.insert({ :id => row*10})] }
      ot: ({'first_error':"Duplicate primary key `id`:\n{\n\t\"foo\":\t3,\n\t\"id\":\t1\n}\n{\n\t\"id\":\t1\n}",'deleted':0.0,'replaced':0.0,'unchanged':0.0,'errors':3,'skipped':0.0,'inserted':3})

    - cd: tbl.count()
      ot: 6

    - cd: r.expr([1, 2, 3]).for_each( tbl2.insert({}) )
      ot: ({'deleted':0.0,'replaced':0.0,'generated_keys':arrlen(3,uuid()),'unchanged':0.0,'errors':0.0,'skipped':0.0,'inserted':3})

    # We have six elements, update them 6*2*3=36 times
    - js: r([1, 2, 3]).forEach(function (row) { return [tbl.update({ foo:row }), tbl.update({ bar:row })] })
      py: r.expr([1,2,3]).for_each(lambda row:[tbl.update({'foo':row}), tbl.update({'bar':row})])
      rb: r.expr([1,2,3]).for_each{ |row| [tbl.update({:foo => row}), tbl.update({:bar => row})]}
      ot: ({'deleted':0.0,'replaced':36,'unchanged':0.0,'errors':0.0,'skipped':0.0,'inserted':0.0})

    # forEach negative cases
    - cd: r.expr([1, 2, 3]).for_each( tbl2.insert({ 'id':r.row }) )
      rb: []
      ot: ({'deleted':0.0,'replaced':0.0,'unchanged':0.0,'errors':0.0,'skipped':0.0,'inserted':3})

    - cd: r.expr([1, 2, 3]).for_each(1)
      ot: 'err("RqlRuntimeError", "FOREACH expects one or more basic write queries.  Expected type ARRAY but found NUMBER.", [0])'

    - py: r.expr([1, 2, 3]).for_each(lambda x:x)
      js: r([1, 2, 3]).forEach(function (x) { return x; })
      rb: r([1, 2, 3]).for_each{ |x| x }
      ot: 'err("RqlRuntimeError", "FOREACH expects one or more basic write queries.  Expected type ARRAY but found NUMBER.", [1, 1])'

    - cd: r.expr([1, 2, 3]).for_each(r.row)
      rb: []
      ot: 'err("RqlRuntimeError", "FOREACH expects one or more basic write queries.  Expected type ARRAY but found NUMBER.", [1, 1])'

    - js: r([1, 2, 3]).forEach(function (row) { return tbl; })
      py: r.expr([1, 2, 3]).for_each(lambda row:tbl)
      rb: r([1, 2, 3]).for_each{ |row| tbl }
      ot: 'err("RqlRuntimeError", "FOREACH expects one or more basic write queries.", [1, 1])'

      # This is only relevant in JS -- what happens when we return undefined
    - js: "r([1, 2, 3]).forEach(function (row) {})"
      cd: []
      ot: err("RqlDriverError", 'Anonymous function returned `undefined`. Did you forget a `return`?', [1])

    # Make sure write queries can't be nested into stream ops
    - cd: r.expr(1).do(tbl.insert({'foo':r.row}))
      rb: r(1).do{ |row| tbl.insert({ :foo => row }) }
      ot: ({'deleted':0.0,'replaced':0.0,'generated_keys':arrlen(1,uuid()),'unchanged':0.0,'errors':0.0,'skipped':0.0,'inserted':1})

    - py: r.expr([1, 2])[0].do(tbl.insert({'foo':r.row}))
      js: r.expr([1, 2]).nth(0).do(tbl.insert({'foo':r.row}))
      rb: r([1, 2])[0].do{ |row| tbl.insert({ :foo => row }) }
      ot: ({'deleted':0.0,'replaced':0.0,'generated_keys':arrlen(1,uuid()),'unchanged':0.0,'errors':0.0,'skipped':0.0,'inserted':1})

    - cd: r.expr([1, 2]).map(tbl.insert({'foo':r.row}))
      rb: r([1, 2]).map{ |row| tbl.insert({ :foo => row }) }
      ot: err('RqlCompileError', 'Cannot nest writes or meta ops in stream operations.  Use FOREACH instead.', [0])

    - cd: r.expr([1, 2]).map(r.db('test').table_create('nested_table'))
      ot: err('RqlCompileError', 'Cannot nest writes or meta ops in stream operations.  Use FOREACH instead.', [0])

    - cd: r.expr(1).do(r.db('test').table_create('nested_table'))
      ot: ({'created':1})

    # Cleanup
    - cd: r.db('test').table_drop('test2')
      ot: ({'dropped':1})

    - cd: r.db('test').table_drop('nested_table')
      ot: ({'dropped':1})
