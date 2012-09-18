// Assumes that rethinkdb is loaded already in whichever environment we happen to be running in

function print(msg) {
    console.log(msg);
}

var q = rethinkdb.query;

var conn;
function testConnect() {
    wait();
    conn = rethinkdb.net.connect({host:HOST, port:PORT}, function() {
        q.db('Welcome-db').list().run(function(tables) {
            wait();
            function drop() {
                var table = tables.shift();
                if (table) {
                    q.db('Welcome-db').drop(table).run(drop);
                } else {
                    q.db('Welcome-db').create('Welcome-rdb').run(function() {
                        done();
                    });
                }
            }
            drop();
        });
        done();
    }, function(e) {
        fail(e);
    });
}

function testBasic() {
    q(1).run(aeq(1));
    q(true).run(aeq(true));
    q('bob').run(aeq('bob'));

    q.expr(1).run(aeq(1));
    q.expr(true).run(aeq(true));
}

function testArith() {
    q(1).add(2).run(aeq(3));
    q(1).sub(2).run(aeq(-1));
    q(5).mul(8).run(aeq(40));
    q(8).div(2).run(aeq(4));
    q(7).mod(2).run(aeq(1));

    q(12).div(3).add(2).mul(3).run(aeq(18));
}

function testCompare() {
    q(1).eq(1).run(aeq(true));
    q(1).eq(2).run(aeq(false));
    q(1).lt(2).run(aeq(true));
    q(8).lt(-4).run(aeq(false));
    q(8).le(8).run(aeq(true));
    q(8).gt(7).run(aeq(true));
    q(8).gt(8).run(aeq(false));
    q(8).ge(8).run(aeq(true));
}

function testBool() {
    q(true).not().run(aeq(false));
    q(true).and(true).run(aeq(true));
    q(true).and(false).run(aeq(false));
    q(true).or(false).run(aeq(true));

    // Test DeMorgan's rule!
    q(true).and(false).eq(q(true).not().or(q(false).not()).not()).run(aeq(true));
}

var arr = q([1,2,3,4,5,6]);
function testSlices() {
    arr.nth(0).run(aeq(1));
    arr.count().run(aeq(6));
    arr.limit(5).count().run(aeq(5));
    arr.skip(4).count().run(aeq(2));
    arr.skip(4).nth(0).run(aeq(5));
    arr.slice(1,4).count().run(aeq(3));
    arr.nth(2).run(aeq(3));
}

function testExtend() {
    q({a:1}).extend({b:2}).run(objeq({a:1,b:2}));
}

function testIf() {
    q.ifThenElse(q(true), q(1), q(2)).run(aeq(1));
    q.ifThenElse(q(false), q(1), q(2)).run(aeq(2));
    q.ifThenElse(q(2).mul(8).ge(q(30).div(2)),
        q(8).div(2),
        q(9).div(3)).run(aeq(4));
}

function testLet() {
    q.let(['a', q(1)], q('$a')).run(aeq(1));
    q.let(['a', q(1)], ['b', q(2)], q('$a').add(q('$b'))).run(aeq(3));
}

function testDistinct() {
    q([1,1,2,3,3,3,3]).distinct().run(objeq([1,2,3]));
}

function testMap() {
    arr.map(q.fn('a', q('$a').add(1))).nth(2).run(aeq(4));
}

function testReduce() {
    arr.reduce(q(0), q.fn('a', 'b', q.R('$a').add(q.R('$b')))).run(aeq(21));
}

function testFilter() {
    arr.filter(function(val) {
        return val.lt(3);
    }).count().run(objeq(2));

    arr.filter(q.fn('a', q.R('$a').lt(3))).count().run(objeq(2));
}

var tobj = q({a:1,b:2,c:3});
function testHasAttr() {
    tobj.hasAttr('a').run(aeq(true));
    tobj.hasAttr('d').run(aeq(false));
}

function testGetAttr() {
    tobj.getAttr('a').run(aeq(1));
    tobj.getAttr('b').run(aeq(2));
    tobj.getAttr('c').run(aeq(3));

    q.let(['a', tobj],
        q.ifThenElse(q('$a').hasAttr('b'),
            q('$a.b'),
            q("No attribute b")
        )
    ).run(aeq(2));
}

function testPickAttrs() {
    tobj.pickAttrs('a').run(objeq({a:1}));
    tobj.pickAttrs(['a', 'b']).run(objeq({a:1,b:2}));
}

function testWithout() {
    tobj.without('a').run(objeq({b:2,c:3}));
    tobj.without(['a','b']).run(objeq({c:3}));
}

function testR() {
    q.let(['a', q({b:1})], q.R('$a.b')).run(aeq(1));
}

var tab = q.table('Welcome-rdb');
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
    tab.orderby('num').nth(2).pickAttrs('num').run(objeq({num:13}));
}

function testPluck() {
    tab.orderby('num').pluck('num').nth(0).run(objeq({num:11}));
}

function testTabFilter() {
    tab.filter(function(row) {
        return row('num').gt(16);
    }).count().run(aeq(4));

    tab.filter(q.fn('row', q.R('$row.num').gt(16))).count().run(aeq(4));

    tab.filter(q.R('num').gt(16)).count().run(aeq(4));

    tab.filter({num:16}).nth(0).run(objeq({id:4,num:16}))

    tab.filter({num:q(20).sub(q.R('id'))}).count().run(aeq(10));
}

function testTabMap() {
    tab.orderby('num').map(q.R('num')).nth(2).run(aeq(13));
}

function testTabReduce() {
    tab.map(q('@.num')).reduce(q(0), q.fn('a','b', q('$b').add(q('$a')))).run(aeq(155));
    tab.map(q('@.num')).reduce(q(0), q.fn('a', 'b', q('$b').add('$a'))).run(aeq(155));
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
    tab.between(2,2).nth(0).run(objeq({
        id:2,
        num:18
    }));
}

function testGroupedMapReduce() {
    tab.groupedMapReduce(function(row) {
        return q.ifThenElse(row('id').lt(5),
            q.expr(0),
            q.expr(1))
    }, function(row) {
        return row('num');
    }, 0, function(acc, num) {
        return acc.add(num);
    }).run(objeq([
            {group:0, reduction:90},
            {group:1, reduction:65}
    ]));
}

function testConcatMap() {
    tab.concatMap(q([1,2])).count().run(aeq(20));
}

var tab2 = q.table('table-2');
function testSetupOtherTable() {
    wait();
    q.db('Welcome-db').create('table-2').run(function() {
        tab2.insert([
            {id:20, name:'bob'},
            {id:19, name:'tom'},
            {id:18, name:'joe'}
        ]).run(objeq({inserted:3}));
        done();
    });
}

function testEqJoin() {
    tab.eqJoin('num', tab2).orderby('id').run(objeq([
        {id:0, num:20, name:'bob'},
        {id:1, num:19, name:'tom'},
        {id:2, num:18, name:'joe'},
    ]));
}

function testDropTable() {
    //q.db('Welcome-db').drop('table-2').run();
}

function testUpdate1() {
    tab.update(function(a) {
        return a.extend({updated:true});
    }).run(objeq({
        errors:0,
        skipped:0,
        updated:10,
    }));
}

function testUpdate2() {
    wait();
    tab.run(function(res) {
        for (var key in res) {
            assertEquals(res[key]['updated'], true);
        }
        done();
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
        return a.pickAttrs('id').extend({mutated:true});
        //return q.expr({id:a.getAttr('id'), mutated:true});
    }).run(objeq({
        deleted:0,
        errors:0,
        inserted:0,
        modified:10
    }));
}

function testMutate2() {
    wait();
    tab.run(function(res) {
        for (var key in res) {
            assertEquals(res[key]['mutated'], true);
            assertEquals(res[key]['updated'], undefined);
        }
        done();
    });
}

function testPointMutate1() {
    tab.get(0).mutate(function(a) {
        return a.pickAttrs('id').extend({pointmutated:true});
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
    q([1,2,3]).forEach(q.fn('a', tab.insert({id:q('$a'), fe:true}))).run(objeq({
        inserted:3
    }));
}

function testForEach2() {
    tab.forEach(q.fn('a', tab.get(q('$a.id')).update(q.expr({fe:true})))).run(objeq({
        updated:3
    }));
}

function testForEach3() {
    wait();
    tab.run(function(res) {
        for (var key in res) {
            assertEquals(res[key]['fe'], true);
        }
        done();
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
    testExtend,
    testIf,
    testLet,
    testDistinct,
    testMap,
    testReduce,
    testFilter,
    testHasAttr,
    testGetAttr,
    testPickAttrs,
    testWithout,
    testR,
    testInsert,
    testGet,
    testOrderby,
    testPluck,
    testTabFilter,
    testTabMap,
    testTabReduce,
    testJS,
    testBetween,
    testGroupedMapReduce,
    testConcatMap,
    testSetupOtherTable,
    testEqJoin,
    testDropTable,
    testUpdate1,
    testUpdate2,
    testPointUpdate1,
    testPointUpdate2,
    testMutate1,
    testMutate2,
    testPointMutate1,
    testPointMutate2,
    testPointDelete1,
    testPointDelete2,
    testDelete1,
    testDelete2,
    testForEach1,
    testForEach2,
    testForEach3,
    testClose,
]);
