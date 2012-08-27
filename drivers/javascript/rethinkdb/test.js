// Assumes that rethinkdb is loaded already in whichever environment we happen to be running in

function print(msg) {
    console.log(msg);
}

var q = rethinkdb.query;
var tab = q.table('Welcome-rdb');

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

function testSlices() {
    var arr = q([1,2,3,4,5,6]);

    arr.length().run(aeq(6));
    arr.limit(5).length().run(aeq(5));
    arr.skip(4).length().run(aeq(2));
    arr.slice(1,4).length().run(aeq(3));
    arr.nth(2).run(aeq(3));
}

function testMap() {
    tab.map(q.R('num')).nth(2).run(aeq(19));
}

function testReduce() {
    tab.reduce(q(0), q.fn('a', 'b', q.R('$a').add(q.R('$b')))).run(aeq(155));
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
    testInsert,
    testGet,
    testSlices,
    testMap,
    testReduce,
    testClose,
]);
