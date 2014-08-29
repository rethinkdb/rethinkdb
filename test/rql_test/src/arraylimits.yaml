desc: Tests array limit variations
table_variable_name: tbl
tests:

  # test simplistic array limits
  - cd: r.expr([1,2,3,4]).union([5, 6, 7, 8])
    runopts:
      array_limit: '8'
    ot: [1,2,3,4,5,6,7,8]
  - cd: r.expr([1,2,3,4]).union([5, 6, 7, 8])
    runopts:
      array_limit: '4'
    ot: err("RqlRuntimeError", "Array over size limit `4`.", [0])

  # test array limits on query creation
  - cd: r.expr([1,2,3,4,5,6,7,8])
    runopts:
      array_limit: '4'
    ot: err("RqlRuntimeError", "Array over size limit `4`.", [0])

  # test bizarre array limits
  - cd: r.expr([1,2,3,4,5,6,7,8])
    runopts:
      array_limit: '-1'
    ot: err("RqlCompileError", "Illegal array size limit `-1`.", [])

  - cd: r.expr([1,2,3,4,5,6,7,8])
    runopts:
      array_limit: '0'
    ot: err("RqlCompileError", "Illegal array size limit `0`.", [])

  # make enormous > 100,000 element array
  - def: ten_l = r.expr([1, 2, 3, 4, 5, 6, 7, 8, 9, 10])
  - def:
      js: ten_f = function(l) { return ten_l }
      py: ten_f = lambda l:list(range(1,11))
  - def:
      js: huge_l = r.expr(ten_l).concatMap(ten_f).concatMap(ten_f).concatMap(ten_f).concatMap(ten_f)
      py: huge_l = r.expr(ten_l).concat_map(ten_f).concat_map(ten_f).concat_map(ten_f).concat_map(ten_f)
      rb: huge_l = r.expr(ten_l).concat_map {|l| ten_l}.concat_map {|l| ten_l}.concat_map {|l| ten_l}.concat_map {|l| ten_l}
  - cd: huge_l.append(1).count()
    runopts:
      array_limit: '100001'
    ot: 100001

  # attempt to insert enormous array
  - cd: tbl.insert({'id':0, 'array':huge_l.append(1)})
    runopts:
      array_limit: '100001'
    ot: ({'deleted':0.0,'replaced':0.0,'unchanged':0.0,'errors':1.0,'skipped':0.0,'inserted':1,'first_error':"Array too large for disk writes (limit 100,000 elements)"})

  - cd: tbl.get(0)
    runopts:
      array_limit: '100001'
    ot: (null)

  # attempt to read array that violates limit from disk
  - cd: tbl.insert({'id':1, 'array':ten_l})
    ot: ({'deleted':0.0,'replaced':0.0,'unchanged':0.0,'errors':0.0,'skipped':0.0,'inserted':1})
  - cd: tbl.get(1)
    runopts:
      array_limit: '4'
    ot: ({'array':[1,2,3,4,5,6,7,8,9,10],'id':1})

