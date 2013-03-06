import rethinkdb as r

c = r.connect(host='localhost', port=28015)

def tests():
    print r.expr(1).run(c)
    print r.expr("bob").run(c)
    print r.expr(True).run(c)
    print r.expr(False).run(c)
    print r.expr(3.12).run(c)
    print r.expr([1,2,3,4,5]).run(c)
    print r.expr({'a':1, 'b':2}).run(c)
    #print r.js('1 + 1').run(c)

    print (r.expr(1) == 2).run(c) # false
    print (r.expr(1) != 2).run(c) # true
    print (r.expr(1) < 2).run(c)  # true
    print (r.expr(1) <= 2).run(c) # true
    print (r.expr(1) > 2).run(c)  # false
    print (r.expr(1) >= 2).run(c) # false
    print (~r.expr(True)).run(c)  # false
    print (~r.expr(False)).run(c) # true

    print (r.expr(1) + 2).run(c) # 3
    print (r.expr(1) - 2).run(c) # -1
    print (r.expr(1) * 2).run(c) # 2
    print (r.expr(1) / 2).run(c) # .5
    print (r.expr(12) % 10).run(c) # 2

    print (((r.expr(12) / 6) * 4) - 3).run(c) # 5

    arr = r.expr([1,2,3,4])

    print arr.append(5).run(c) 
    print arr[1].run(c)
    print arr[2].run(c)
    print arr[1:2].run(c)
    print arr[:2].run(c)
    print arr[2:].run(c)
    print arr.count().run(c)
    print arr.union(arr).run(c)
    print arr.union(arr).distinct().run(c)
    print arr.inner_join(arr, lambda a,b: a == b).run(c)
    print arr.outer_join(arr, lambda a,b: a == (b - 2)).run(c)

    #print r.expr([{'id':0, 'a':0}, {'id':1, 'a':0}]).eq_join([{'id':0, 'b':1}, {'id':1, 'b':1}], 'id').run(c)

    obj = r.expr({'a':1, 'b':2})

    print obj['a'].run(c)
    print obj.contains('a').run(c)
    print obj.pluck('a').run(c)
    print obj.without('a').run(c)
    print obj.merge({'c':3}).run(c)

    print r.db_list().run(c)
    print r.db_create('bob').run(c)
    print r.db_create('test').run(c)
    print r.db_list().run(c)
    print r.db('test').table_list().run(c)
    print r.db('test').table_create('test').run(c)
    print r.db('test').table_create('bob').run(c)
    print r.db('test').table_list().run(c)
    print r.db('test').table_drop('bob').run(c)
    print r.db('test').table_list().run(c)

    test = r.db('test').table('test')

    print test.run(c)
    print test.insert({'id': 1, 'a': 2}).run(c)
    print test.insert({'id': 2, 'a': 3}).run(c)
    print test.insert({'id': 3, 'a': 4}).run(c)
    print test.run(c)
    print test.between(right_bound=2).run(c)

    print test.update(lambda row: {'a': row['a']+1}).run(c)
    print test.run(c)
    print test.replace(lambda row: {'id':row['id'], 'a': row['a']+1}).run(c)
    print test.run(c)
    print test.delete().run(c)
    print test.run(c)

    print r.expr(1).do(lambda a: a + 1).run(c)
    print r.expr(2).do(lambda a: {'b': a / a}).run(c)
    print r.expr([1,2,3]).map(lambda a: a + 1).run(c)
    print r.expr([1,2,3]).map(lambda a: a.do(lambda b: b+a)).run(c)
    print r.expr([1,2,3]).reduce(lambda a, b: a+b).run(c)
    print r.expr([1,2,3,4]).filter(lambda a: a < 3).run(c)

    print r.expr([1,2]).concat_map(lambda a: [a,a]).run(c)

    print r.branch(r.expr(1) < 2, "a", "b").run(c)
    print r.branch(r.expr(1) < 0, "a", "b").run(c)

    print (r.expr(True) & r.expr(False)).run(c)
    print (r.expr(True) | r.expr(False)).run(c)
    print (r.expr(True) & r.expr(True)).run(c)
    print (r.expr(False) | r.expr(False)).run(c)

    #print r.expr([1,2]).map(3).run(c)
    #print r.expr([1,2]).map(r.row + 3).run(c)
    print r.expr([{'id':2}, {'id':3}, {'id':1}]).order_by('id').run(c)
    print r.expr([{'g':0, 'v':1}, {'g':0, 'v':2}, {'g':1, 'v':1}, {'g':1, 'v':2}]).grouped_map_reduce(lambda row: row['g'], lambda row: row['v'] + 1, lambda a,b: a + b).run(c)

    #print r.expr([1,2]).for_each(lambda i: [test.insert({'id':i, 'a': i+1})]).run(c)
    print test.run(c)

class except_printer:
    def __enter__(self):
        pass

    def __exit__(self, typ, value, traceback):
        print value
        return True

def go():
    with except_printer():
        r.expr({'err': r.error('bob')}).run(c)
    with except_printer():
        r.expr([1,2,3, r.error('bob')]).run(c)
    with except_printer():
        (((r.expr(1) + 1) - 8) * r.error('bob')).run(c)
    with except_printer():
        r.expr([1,2,3]).append(r.error('bob')).run(c)
    with except_printer():
        r.expr([1,2,3, r.error('bob')])[1:].run(c)
    with except_printer():
        r.expr({'a':r.error('bob')})['a'].run(c)
    with except_printer():
        r.db('test').table('test').filter(lambda a: a.contains(r.error('bob'))).run(c)
    with except_printer():
        r.expr(1).do(lambda x: r.error('bob')).run(c)
    with except_printer():
        r.expr(1).do(lambda x: x + r.error('bob')).run(c)
    with except_printer():
        r.branch(r.db('test').table('test').get(0)['a'].contains(r.error('bob')), r.expr(1), r.expr(2)).run(c)
    with except_printer():
        r.expr([1,2]).reduce(lambda a,b: a + r.error("bob")).run(c)
    #with except_printer():
    #    r.expr([1,2,3]).do(lambda a: a.map(lambda b: a.filter(lambda c: (b + c) % a.reduce(base=9, lambda d, e: d / r.branch(e < 4, "bob", r.error('deep')))))).run(c)

for db in r.db_list().run(c):
    r.db_drop(db).run(c)

tests()
go()
