// Assumes that rethinkdb is loaded already in whichever environment we happen to be running in

function print(msg) {
    console.log(msg);
}

var r = rethinkdb;

var conn;
function testConnect() {
    wait();
    conn = r.connect({host:HOST, port:PORT}, function() {
        r.db('test').list().run(function(table) {
            if (table) {
                r.db('test').drop(table).run();
                return true;
            } else {
                r.db('test').create('Welcome-rdb').run(function() {
                    done();
                });
                return false;
            }
        });
    }, function(e) {
        fail(e);
    });
}

function testBasic() {
    r.expr(1).run(aeq(1));
    r.expr(true).run(aeq(true));
    r.expr('bob').run(aeq('bob'));
}

function testArith() {
    r.expr(1).add(2).run(aeq(3));
    r.expr(1).sub(2).run(aeq(-1));
    r.expr(5).mul(8).run(aeq(40));
    r.expr(8).div(2).run(aeq(4));
    r.expr(7).mod(2).run(aeq(1));

    r.expr(12).div(3).add(2).mul(3).run(aeq(18));
}

function testCompare() {
    r.expr(1).eq(1).run(aeq(true));
    r.expr(1).eq(2).run(aeq(false));
    r.expr(1).lt(2).run(aeq(true));
    r.expr(8).lt(-4).run(aeq(false));
    r.expr(8).le(8).run(aeq(true));
    r.expr(8).gt(7).run(aeq(true));
    r.expr(8).gt(8).run(aeq(false));
    r.expr(8).ge(8).run(aeq(true));
}

function testBool() {
    r.expr(true).not().run(aeq(false));
    r.expr(true).and(true).run(aeq(true));
    r.expr(true).and(false).run(aeq(false));
    r.expr(true).or(false).run(aeq(true));

    // Test DeMorgan's rule!
    r.expr(true).and(false).eq(r.expr(true).not().or(r.expr(false).not()).not()).run(aeq(true));
}

var arr = r.expr([1,2,3,4,5,6]);
function testSlices() {
    arr.nth(0).run(aeq(1));
    arr.count().run(aeq(6));
    arr.limit(5).count().run(aeq(5));
    arr.skip(4).count().run(aeq(2));
    arr.skip(4).nth(0).run(aeq(5));
    arr.slice(1,4).count().run(aeq(3));
    arr.nth(2).run(aeq(3));
}

function testAppend() {
    arr.append(7).nth(6).run(aeq(7));
}

function testExtend() {
    r.expr({a:1}).extend({b:2}).run(objeq({a:1,b:2}));
}

function testIf() {
    r.ifThenElse(r.expr(true), r.expr(1), r.expr(2)).run(aeq(1));
    r.ifThenElse(r.expr(false), r.expr(1), r.expr(2)).run(aeq(2));
    r.ifThenElse(r.expr(2).mul(8).ge(r.expr(30).div(2)),
        r.expr(8).div(2),
        r.expr(9).div(3)).run(aeq(4));
}

function testLet() {
    r.let({a:r.expr(1)}, r.letVar('a')).run(aeq(1));
    r.let({a:r.expr(1), b:r.expr(2)}, r.letVar('a').add(r.letVar('b'))).run(aeq(3));
}

function testDistinct() {
    var cursor = r.expr([1,1,2,3,3,3,3]).distinct().run(objeq(1,2,3));
}

function testMap() {
    arr.map(function(a) {return a.add(1)}).nth(2).run(aeq(4));
}

function testReduce() {
    arr.reduce(0, function(a, b) {return a.add(b)}).run(aeq(21));
}

function testFilter() {
    arr.filter(function(val) {
        return val.lt(3);
    }).count().run(objeq(2));
}

var tobj = r.expr({a:1,b:2,c:3});
function testContains() {
    tobj.contains('a').run(aeq(true));
    tobj.contains('d').run(aeq(false));
}

function testGetAttr() {
    tobj.getAttr('a').run(aeq(1));
    tobj.getAttr('b').run(aeq(2));
    tobj.getAttr('c').run(aeq(3));

    r.let({a:tobj},
        r.ifThenElse(r.letVar('a').contains('b'),
            r.letVar('a.b'),
            r.error("No attribute b")
        )
    ).run(aeq(2));
}

function testPickAttrs() {
    tobj.pick('a').run(objeq({a:1}));
    tobj.pick('a', 'b').run(objeq({a:1,b:2}));
}

function testUnpick() {
    tobj.unpick('a').run(objeq({b:2,c:3}));
    tobj.unpick(['a','b']).run(objeq({c:3}));
}

function testR() {
    r.let({a:r.expr({b:1})}, r.letVar('a.b')).run(aeq(1));
}

var tab = r.table('Welcome-rdb');
function testInsert() {
    tab.insert({id:0, num:20}).run(objeq({inserted:1}));

    var others = [];
    for (var i = 1; i < 10; i++) {
        others.push({id:i, num:20-i});
    }

    tab.insert(others).run(objeq({inserted:9}));
}

function testGet() {
    for (var i = 0; i < 10; i++) {
        tab.get(i).run(objeq({id:i,num:20-i}));
    }
}

function testOrderby() {
    tab.orderby('num').nth(2).run(objeq({id:7,num:13}));
    tab.orderby('num').nth(2).pick('num').run(objeq({num:13}));
}

function testPluck() {
    tab.orderby('num').pluck('num').nth(0).run(objeq({num:11}));
}

function testWithout() {
    tab.orderby('num').without('num').nth(0).run(objeq({id:9}));
}

function testUnion() {
    r.expr([1,2,3]).union([4,5,6]).run(objeq(1,2,3,4,5,6));
    tab.union(tab).count().eq(tab.count().mul(2)).run(aeq(true));
}

function testTabFilter() {
    tab.filter(function(row) {
        return row('num').gt(16);
    }).count().run(aeq(4));

    tab.filter(function(row) {return row('num').gt(16)}).count().run(aeq(4));

    tab.filter(r('num').gt(16)).count().run(aeq(4));

    tab.filter({num:16}).nth(0).run(objeq({id:4,num:16}));

    tab.filter({num:r.expr(20).sub(r('id'))}).count().run(aeq(10));
}

function testTabMap() {
    tab.orderby('num').map(r('num')).nth(2).run(aeq(13));
}

function testTabReduce() {
    tab.map(r('num')).reduce(r.expr(0), function(a,b) {return b.add(a)}).run(aeq(155));
    tab.map(r('num')).reduce(r.expr(0), function(a,b) {return b.add(a)}).run(aeq(155));
}

function testJS() {
    tab.filter(function(row) {
        return row('num').gt(16);
    }).count().run(aeq(4));

    tab.map(function(row) {
        return row('num').add(2);
    }).filter(function (val) {
        return val.gt(16);
    }).count().run(aeq(6));

    tab.filter(function(row) {
        return row('num').gt(16);
    }).map(function(row) {
        return row('num').mul(4);
    }).reduce(0, function(acc, val) {
        return acc.add(val);
    }).run(aeq(296));
}

function testBetween() {
    tab.between(2,3).count().run(aeq(2));
    tab.between(2,3).orderby('id').nth(0).run(objeq({
        id:2,
        num:18
    }));
}

function testGroupedMapReduce() {
    tab.groupedMapReduce(function(row) {
        return r.ifThenElse(row('id').lt(5),
            r.expr(0),
            r.expr(1))
    }, function(row) {
        return row('num');
    }, 0, function(acc, num) {
        return acc.add(num);
    }).run(objeq(
        {group:0, reduction:90},
        {group:1, reduction:65}
    ));
}

function testGroupBy() {
    var s = r.expr([
        {g1: 1, g2: 1, num: 0},
        {g1: 1, g2: 2, num: 5},
        {g1: 1, g2: 2, num: 10},
        {g1: 2, g2: 3, num: 0},
        {g1: 2, g2: 3, num: 100}
    ]);

    s.groupBy('g1', r.average('num')).run(objeq(
        {group:1, reduction:5},
        {group:2, reduction:50}
    ));
    s.groupBy('g1', r.count).run(objeq(
        {group:1, reduction:3},
        {group:2, reduction:2}
    ));
    s.groupBy('g1', r.sum('num')).run(objeq(
        {group:1, reduction:15},
        {group:2, reduction:100}
    ));
    s.groupBy('g1', 'g2', r.average('num')).run(objeq(
        {group:[1,1], reduction: 0},
        {group:[1,2], reduction: 7.5},
        {group:[2,3], reduction: 50}
    ));
}

function testConcatMap() {
    tab.concatMap(r.expr([1,2])).count().run(aeq(20));
}

function testJoin1() {
    var s1 = r.expr([{id:0, name:'bob'}, {id:1, name:'tom'}, {id:2, name:'joe'}]);
    var s2 = r.expr([{id:0, title:'goof'}, {id:2, title:'lmoe'}]);

    wait();
    r.db('test').create('joins1').run(function() {
        r.table('joins1').insert(s1).run(done);
    });

    wait();
    r.db('test').create('joins2').run(function() {
        r.table('joins2').insert(s2).run(done);
    });
}

function testJoin2() {
    var s1 = r.table('joins1');
    var s2 = r.table('joins2');

    s1.innerJoin(s2, function(one, two) {
        return one('id').eq(two('id'));
    }).zip().orderby('id').run(objeq(
        {id:0, name: 'bob', title: 'goof'},
        {id:2, name: 'joe', title: 'lmoe'}
    ));

    s1.outerJoin(s2, function(one, two) {
        return one('id').eq(two('id'));
    }).zip().orderby('id').run(objeq(
        {id:0, name: 'bob', title: 'goof'},
        {id:1, name: 'tom'},
        {id:2, name: 'joe', title: 'lmoe'}
    ));

    s1.equiJoin('id', r.table('joins2')).zip().orderby('id').run(objeq(
        {id:0, name: 'bob', title: 'goof'},
        {id:2, name: 'joe', title: 'lmoe'}
    ));
}

var tab2 = r.table('table-2');
function testSetupOtherTable() {
    wait();
    r.db('test').create('table-2').run(function() {
        tab2.insert([
            {id:20, name:'bob'},
            {id:19, name:'tom'},
            {id:18, name:'joe'}
        ]).run(objeq({inserted:3}));
        done();
    });
}

function testDropTable() {
    r.db('test').drop('table-2').run();
}

function testUpdate1() {
    tab.filter(function(row) {
        return row('id').ge(5);
    }).update(function(a) {
        return a.extend({updated:true});
    }).run(objeq({
        errors:0,
        skipped:0,
        updated:5,
    }));

    tab.filter(function(row) {
        return row('id').lt(5);
    }).update({updated:true}).run(objeq({
        errors:0,
        skipped:0,
        updated:5,
    }));
}

function testUpdate2() {
    wait();
    tab.run(function(row) {
        if (row === undefined) {
            done();
            return false;
        } else {
            assertEquals(row['updated'], true);
            return true;
        }
    });
}

function testPointUpdate1() {
    tab.get(0).update(function(a) {
        return a.extend({pointupdated:true});
    }).run(objeq({
        errors:0,
        skipped:0,
        updated:1
    }));
}

function testPointUpdate2() {
    tab.get(0)('pointupdated').run(aeq(true));
}

function testMutate1() {
    tab.mutate(function(a) {
        return a.pick('id').extend({mutated:true});
    }).run(objeq({
        deleted:0,
        errors:0,
        inserted:0,
        modified:10
    }));
}

function testMutate2() {
    wait();
    tab.run(function(row) {
        if (row === undefined) {
            done();
            return false;
        } else {
            assertEquals(row['mutated'], true);
            return true;
        }
    });
}

function testPointMutate1() {
    tab.get(0).mutate(function(a) {
        return a.pick('id').extend({pointmutated:true});
        //return {id:a.id, pointmutated:true};
    }).run(objeq({
        deleted:0,
        errors:0,
        inserted:0,
        modified:1
    }));
}

function testPointMutate2() {
    tab.get(0)('pointmutated').run(aeq(true));
}

var docs = [0,1,2,3,4,5,6,7,8,9];
docs = docs.map(function(x) {return {id:x}});
var tbl = r.table('tbl');

function testSetupDetYNonAtom() {
    wait();
    r.db('test').create('tbl').run(function() {
        tbl.insert(docs).run(function() {
            done();
        });
    });
}

function testDet() {
    tbl.update(function(row) {return r.expr({count: r.js('return 0')})}).run(
        objeq({errors:10, updated:0, skipped:0}));
    tbl.update(function(row) {return r.expr({count: 0})}).run(
        objeq({errors:0, updated:10, skipped:0}));

    wait();
    tbl.mutate(function(row) {return tbl.get(row('id'))}).run(function(res) {
        assertEquals(res.errors, 10);
        done();
    });
    wait();
    tbl.mutate(function(row) {return row}).run(function(possible_err) {
        assertEquals(possible_err instanceof Error, false);
        done();
    });

    wait();
    tbl.update({count: tbl.map(function(x) {
        return x('count')
    }).reduce(0, function(a,b) {
        return a.add(b);
    })}).run(function(res) {
        assertEquals(res.errors, 10);
        done();
    });

    wait();
    tbl.update({count: r.expr(docs).map(function(x) {
        return x('id')
    }).reduce(0, function(a,b) {
        return a.add(b);
    })}).run(function(res) {
        assertEquals(res.updated, 10);
        done();
    });
}

function testNonAtomic() {
    var rerr = rethinkdb.errors.RuntimeError;

    // Update modify
    tbl.update(function(row) {return r.expr({x: r.js('return 1')})}).run(attreq('errors', 10));
    tbl.update(function(row) {return r.expr({x:r.js('return 1')})}, true).run(attreq('updated', 10));
    tbl.map(function(row) {return row('x')}).reduce(0, function(a,b) {return a.add(b)}).run(aeq(10));

    tbl.get(0).update(function(row) {return r.expr({x: r.js('return 1')})}).run(atype(rerr));
    tbl.get(0).update(function(row) {return r.expr({x: r.js('return 2')})}, true).run(
        attreq('updated', 1));
    tbl.map(function(a){return a('x')}).reduce(0, function(a,b){return a.add(b);}).run(aeq(11));

    // Update error
    tbl.update(function(row){return r.expr({x:r.js('return x')})}).run(attreq('errors', 10));
    tbl.update(function(row){return r.expr({x:r.js('return x')})}, true).run(attreq('errors', 10));
    tbl.map(function(a){return a('x')}).reduce(0, function(a,b){return a.add(b);}).run(aeq(11));
    tbl.get(0).update(function(row){return r.expr({x:r.js('x')})}).run(atype(rerr));
    tbl.get(0).update(function(row){return r.expr({x:r.js('x')})}, true).run(atype(rerr));
    tbl.map(function(a){return a('x')}).reduce(0, function(a,b){return a.add(b);}).run(aeq(11));

    // Update skipped
    tbl.update(function(row){return r.ifThenElse(r.js('return true'),
                                        r.expr(null),
                                        r.expr({x:0.1}))}).run(attreq('errors',10));
    tbl.update(function(row){return r.ifThenElse(r.js('return true'),
                                        r.expr(null),
                                        r.expr({x:0.1}))}, true).run(attreq('skipped',10));
    tbl.map(function(a){return a('x')}).reduce(0, function(a,b){return a.add(b);}).run(aeq(11));
    tbl.get(0).update(function(row){return r.ifThenElse(r.js('return true'),
                                            r.expr(null),
                                            r.expr({x:0.1}))}).run(atype(rerr));
    tbl.get(0).update(function(row){return r.ifThenElse(r.js('return true'),
                                            r.expr(null),
                                            r.expr({x:0.1}))}, true).run(attreq('skipped', 1));
    tbl.map(function(a){return a('x')}).reduce(0, function(a,b){return a.add(b);}).run(aeq(11));

    // Mutate modify
    tbl.get(0).mutate(function(row){return r.ifThenElse(r.js('return true'), row, r.expr(null))}).run(
        atype(rerr));
    tbl.get(0).mutate(function(row){return r.ifThenElse(r.js('return true'), row, r.expr(null))},
        true).run(attreq('modified', 1));
    tbl.map(function(a){return a('x')}).reduce(0, function(a,b){return a.add(b);}).run(aeq(11));
    tbl.mutate(r.fn('rowA', r.ifThenElse(r.js('return rowA.id == 1'), r.letVar('rowA').extend({x:2}),
        r.letVar('rowA')))).run(attreq('errors', 10));
    tbl.mutate(r.fn('rowA', r.ifThenElse(r.js('return rowA.id == 1'), r.letVar('rowA').extend({x:2}),
        r.letVar('rowA'))), true).run(attreq('modified', 10));
    tbl.map(function(a){return a('x')}).reduce(0, function(a,b){return a.add(b);}).run(aeq(12));

    // Mutate error
    tbl.get(0).mutate(function(row){return r.ifThenElse(r.js('return x'), row, r.expr(null))}).run(
        atype(rerr));
    tbl.get(0).mutate(function(row){return r.ifThenElse(r.js('return x'), row, r.expr(null))},
        true).run(atype(rerr));
    tbl.map(function(a){return a('x')}).reduce(0, function(a,b){return a.add(b);}).run(aeq(12));

    // Mutate delete
    tbl.get(0).mutate(function(row){return r.ifThenElse(r.js('return true'), r.expr(null), row)}).run(
        atype(rerr));
    tbl.get(0).mutate(function(row){return r.ifThenElse(r.js('return true'), r.expr(null), row)},
        true).run(attreq('deleted', 1));
    tbl.map(function(a){return a('x')}).reduce(0, function(a,b){return a.add(b);}).run(aeq(10));
    tbl.mutate(r.fn('rowA', r.ifThenElse(r.js('return rowA.id < 3'), r.expr(null),
        r.letVar('rowA')))).run(attreq('errors', 9));
    tbl.mutate(r.fn('rowA', r.ifThenElse(r.js('return rowA.id < 3'), r.expr(null),
        r.letVar('rowA'))), true).run(attreq('deleted', 2));
    tbl.map(function(a){return a('x')}).reduce(0, function(a,b){return a.add(b);}).run(aeq(7));

    // Mutate insert
    tbl.get(0).mutate({id:0, count:tbl.get(3)('count'), x:tbl.get(3)('x')}).run(atype(rerr));
    tbl.get(0).mutate({id:0, count:tbl.get(3)('count'), x:tbl.get(3)('x')}, true).run(
        attreq('inserted', 1));
    tbl.get(1).mutate(tbl.get(3).extend({id:1})).run(atype(rerr));
    tbl.get(1).mutate(tbl.get(3).extend({id:1}), true).run(attreq('inserted', 1));
    tbl.get(2).mutate(tbl.get(1).extend({id:2}), true).run(attreq('inserted', 1));
    tbl.map(function(a){return a('x')}).reduce(0, function(a,b){return a.add(b);}).run(aeq(10));
}

function testPointDelete1() {
    tab.get(0).del().run(objeq({
        deleted:1
    }));
}

function testPointDelete2() {
    tab.count().run(aeq(9));
}

function testDelete1() {
    tab.count().run(aeq(9));
    tab.del().run(objeq({deleted:9}));
}

function testDelete2() {
    tab.count().run(aeq(0));
}

function testForEach1() {
    r.expr([1,2,3]).forEach(function(a) {return tab.insert({id:a, fe:true})}).run(objeq({
        inserted:3
    }));
}

function testForEach2() {
    tab.forEach(function(a) {return tab.get(a('id')).update(r.expr({fe:true}))}).run(objeq({
        updated:3
    }));
}

function testForEach3() {
    wait();
    tab.run(function(row) {
        if (row === undefined) {
            done();
            return false;
        } else {
            assertEquals(row['fe'], true);
            return true;
        }
    });
}

function testClose() {
    tab.del().run(function() {
        conn.close();
    });
}

runTests([
    testConnect,
    testBasic,
    testCompare,
    testArith,
    testBool,
    testSlices,
    testAppend,
    testExtend,
    testIf,
    testLet,
    testDistinct,
    testMap,
    testReduce,
    testFilter,
    testContains,
    testGetAttr,
    testPickAttrs,
    testUnpick,
    testR,
    testInsert,
    testGet,
    testOrderby,
    testPluck,
    testWithout,
    testUnion,
    testTabFilter,
    testTabMap,
    testTabReduce,
    testJS,
    testBetween,
    testGroupedMapReduce,
    testGroupBy,
    testConcatMap,
    testJoin1,
    testJoin2,
    testSetupOtherTable,
    testDropTable,
    testUpdate1,
    testUpdate2,
    testPointUpdate1,
    testPointUpdate2,
    testMutate1,
    testMutate2,
    testPointMutate1,
    testPointMutate2,
    testSetupDetYNonAtom,
    testDet,
    testNonAtomic,
    testPointDelete1,
    testPointDelete2,
    testDelete1,
    testDelete2,
    testForEach1,
    testForEach2,
    testForEach3,
    testClose,
]);
