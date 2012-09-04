// Assumes that rethinkdb is loaded already in whichever environment we happen to be running in

function print(msg) {
    console.log(msg);
}

var q = rethinkdb.query;

var conn;
function testConnect() {
    wait();
    conn = rethinkdb.net.connect('newton', function() {
        done();
    }, function() {
        fail("Could not connect");
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
    arr.length().run(aeq(6));
    arr.limit(5).length().run(aeq(5));
    arr.skip(4).length().run(aeq(2));
    arr.skip(4).nth(0).run(aeq(5));
    arr.slice(1,4).length().run(aeq(3));
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
    // Awaiting changes on server
    //arr.reduce(q(0), q.fn('a', 'b', q.R('$a').add(q.R('$b')))).run(aeq(21));
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
    q({a:1,b:2,c:3}).pickAttrs('a').run(objeq({a:1}));
    q({a:1,b:2,c:3}).pickAttrs(['a', 'b']).run(objeq({a:1,b:2}));
}

function testR() {
    q.let(['a', q({b:1})], q.R('$a.b')).run(aeq(1));
}

var tab = q.table('Welcome-rdb');
function testInsert() {
    for (var i = 0; i < 10; i++) {
        tab.insert({id:i, num:20-i}).run(objeq({inserted:1}));
    }
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

function testTabMap() {
    tab.orderby('num').map(q.R('num')).nth(2).run(aeq(13));
}

function testTabReduce() {
    tab.map(q('@.num')).reduce(q(0), q.fn('a','b', q('$b').add(q('$a')))).run(aeq(155));
    tab.map(q('@.num')).reduce(q(0), q.fn('a', 'b', q('$b').add('$a'))).run(aeq(155));
}

function testJS() {
    tab.filter(function(row) {
        return row.num > 16;
    }).length().run(aeq(4));

    tab.map(function(row) {
        return row.num + 2;
    }).filter(function (val) {
        return val > 16;
    }).length().run(aeq(6));

    // Crashes the server
    /*
    tab.filter(function(row) {
        return row.num > 16;
    }).map(function(row) {
        return row.num * 4;
    }).reduce(0, function(acc, val) {
        return acc + val;
    }).run(aeq(296));
    */
}

function testBetween() {
    tab.between(2,3).length().run(aeq(2));
    tab.between(2,2).nth(0).run(objeq({
        id:2,
        num:18
    }));
}

function testUpdate1() {
    tab.update(function(a) {
        a.updated = true;
        return a;
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

function testMutate1() {
    tab.mutate(function(a) {
        return {id:a.id, mutated:true};
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

function testDelete1() {
    tab.length().run(aeq(10));
    tab.del().run(objeq({deleted:10}));
}

function testDelete2() {
    tab.length().run(aeq(0));
}

function testClose() {
    conn.close();
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
    testHasAttr,
    testGetAttr,
    testPickAttrs,
    testInsert,
    testGet,
    testOrderby,
    testPluck,
    testTabMap,
    testTabReduce,
    testJS,
    testBetween,
    testUpdate1,
    testUpdate2,
    testMutate1,
    testMutate2,
    testDelete1,
    testDelete2,
    testClose,
]);
