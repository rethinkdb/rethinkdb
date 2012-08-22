// Assumes that rethinkdb is loaded already in whichever environment we happen to be running in

/* Testing utilies */

var red = '\u001b[31m';
var green = '\u001b[32m';
var purple = '\u001b[35m';
var reset = '\u001b[0m';

function log(msg) {
    console.log(msg);
}

function err(msg) {
    console.log('   '+red+msg+reset);
}

function poslog(msg) {
    console.log('   '+green+msg+reset);
}

function fail(msg) {
    err('failed with message: '+msg);
    console.trace();
    exit();
}

function assertEquals(expected, val) {
    if (expected !== val) {
        fail("Value "+val+" expected to equal "+expected);
    }
}

function aeq(eq) {
    wait();
    return function(val) {
        assertEquals(eq, val);
        done();
    };
}

var currentTest = '';
var waitCount = 0;
function wait() {
    waitCount++;
}

function done() {
    waitCount--;
}

function runTests(tests) {
    function runTest(testName) {
        if (!testName) return;
        currentTest = testName;

        function cont() {
            poslog("passed");
            runTest(tests.shift());
        }

        log("Running test: "+testName);
        try {
            eval(testName)();
            if (waitCount > 0) {
                setTimeout(function() {
                    if (waitCount > 0) {
                        fail('timedout');
                    } else {
                        cont();
                    }
                }, 200);
            } else {
                cont();
            }
        } catch (e) {
            fail('threw exception '+e);
        }
    }
    runTest(tests.shift());
}

/* Actual tests */
var q = rethinkdb.query;

var conn;
function testConnect() {
    wait();
    conn = rethinkdb.net.connect('localhost', function() {
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

function testClose() {
    conn.close();
}

runTests([
    'testConnect',
    'testBasic',
    'testCompare',
    'testArith',
    'testBool',
    'testClose',
]);
