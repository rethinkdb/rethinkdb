var r = require('./../../../drivers/javascript/build/rethinkdb.js');

var JSPORT = process.argv[2]
var CPPPORT = process.argv[3]

// -- utilities --

failure_count = 0;

function printTestFailure(name, src, messages) {
    failure_count += 1;
    console.log.apply(console,["\nTEST FAILURE: "+name+"\nTEST BODY: "+src+"\n"].concat(messages).concat(["\n"]));
}

function eq_test(one, two) {
    TRACE("eq_test");
    if (one instanceof Function) {
        return one(two);
    } else if (two instanceof Function) {
        return two(one);
    } else if (goog.isArray(one)) {

        if (!goog.isArray(two)) return false;

        if (one.length != two.length) return false;

        // Recurse on each element of array
        for (var i = 0; i < one.length; i++) {
            if (!eq_test(one[i], two[i])) return false;
        }

        return true;

    } else if (one instanceof Object) {

        if (!(two instanceof Object)) return false;

        // TODO: eq_test({},{foo:4}) will return true
        // Recurse on each property of object
        for (var key in one) {
            if (one.hasOwnProperty(key)) {
                if (!eq_test(one[key], two[key])) return false;
            }
        }
        return true;

    } else {

        // Primitive comparison
        return (typeof one === typeof two) && (one === two)
    }
}

// -- Curried output test functions --

// Equality comparison
function eq(exp) {
    var fun = function(val) {
        if (!eq_test(val, exp)) {
            return false;
        } else {
            return true;
        }
    };
    fun.toString = function() {
        return exp.toString();
    };
    return fun;
}

// Tests are stored in list until they can be sequentially evaluated
var tests = []

// Any variables defined by the tests are stored here
var defines = {}

function TRACE(){
    //console.log.apply(console, ["TRACE"].concat([].splice.call(arguments, 0)));
}

// Connect first to cpp server
r.connect({port:CPPPORT}, function(cpp_conn_err, cpp_conn) {

       if(cpp_conn_err || !cpp_conn){
           console.log("Failed to connect to server:", cpp_conn_err);
           process.exit(1);

       }

        // Pull a test off the queue and run it
        function runTest() { try {
            TRACE("runTest");
            var testPair = tests.shift();
            if (testPair) {
                TRACE("!!testPair")
                if (testPair instanceof Function) {
                    TRACE("testPair is fun")
                    testPair();
                    runTest();
                    return;
                } else {
                    TRACE("testPair is not fun")

                    testName = testPair[2];
                    TRACE(testName);
                    exp_val = testPair[1];
                    var exp_fun = eval(testPair[1]);
                    if (!exp_fun)
                        exp_fun = function() { return true; };

                    if (!(exp_fun instanceof Function))
                        exp_fun = eq(exp_fun);

                    var src = testPair[0]
                    try {
                        with (defines) {
                            TRACE("before eval");
                            test = eval(src);
                            TRACE("after eval");
                        }
                    } catch(bld_err) {
                        if (exp_fun.isErr) {
                            if (!exp_fun(bld_err)) {
                                printTestFailure(testName, src,
                                    ["Error eval'ing test src not equal to expected err:\n\tERROR: ",
                                        bld_err,"\n\tExpected: ", exp_fun]);
                            }
                        } else {
                            printTestFailure(testName, src, ["Error eval'ing test src:\n\t", bld_err]);
                        }

                        // continue to next test
                        runTest();
                        return;
                    }

                    // Run test first on cpp server
                    try {
                        TRACE("run",test);
                        test.run(cpp_conn, cpp_cont);
                    } catch(err) {
                        if (exp_fun.isErr) {
                            if (!exp_fun(err)) {
                                printTestFailure(testName, src,
                                    ["Error running test not equal to expected err:\n\tERROR: ",
                                        err, "\n\tEXPECTED: ", exp_val]);
                            }
                        } else {
                            printTestFailure(testName, src, ["Error running test:\n\t",err]);
                        }

                        // Continue to next test
                        runTest();
                        return;
                    }

                    function cpp_cont(cpp_err, cpp_res_cursor) { try {
                        TRACE("cpp_cont");

                        if (cpp_err) {
                            afterArray(cpp_err, null);
                        } else if (cpp_res_cursor instanceof Object && cpp_res_cursor.toArray) {
                            cpp_res_cursor.toArray(afterArray);
                        } else {
                            afterArray(null, cpp_res_cursor);
                        }

                        function afterArray(cpp_err, cpp_res) { try {
                            TRACE("afterArray");

                            if (cpp_err) {
                                if (exp_fun.isErr) {
                                    if (!exp_fun(cpp_err)) {
                                        printTestFailure(testName, src,
                                                         ["Error running test on CPP server not equal to expected err:",
                                                          "\n\tERROR: ",cpp_err,
                                                          "\n\tEXPECTED ",exp_val]);
                                    }
                                } else {
                                    printTestFailure(testName, src,
                                                     ["Error running test on CPP server:",
                                                      "\n\tERROR: ",cpp_err]);
                                }
                            } else if (!exp_fun(cpp_res)) {
                                printTestFailure(testName, src,
                                                 ["CPP result is not equal to expected result:",
                                                  "\n\tVALUE: ",cpp_res,"\n\tEXPECTED: ",exp_val]);
                            }

                            // Continue to next test. Tests are fully sequential
                            // so you can rely on previous queries results in
                            // subsequent tests.
                            runTest();
                            return;
                        } catch(err) {
                            unexpectedException("afterArray", testName, err);
                        } }
                    } catch(err) {
                        unexpectedException("cpp_cont", testName, err);
                    } }
                }
            } else {
                // We've hit the end of our test list
                // closing the connection will allow the
                // event loop to quit naturally
                cpp_conn.close();

                if(failure_count != 0){
                    console.log("Failed " + failure_count + " tests");
                    process.exit(1);
                } else {
                    console.log("Passed all tests")
                }
            }
        } catch (err) {
            unexpectedException("runTest", testName, testPair[1], err);
        } }

        // Start the recursion though all the tests
        runTest();
});

function unexpectedException(){
    console.log("Oops, this shouldn't have happened:");
    console.log.apply(console, arguments);
    process.exit(1);
}

// Invoked by generated code to add test and expected result
// Really constructs list of tests to be sequentially evaluated
function test(testSrc, resSrc, name) {
    tests.push([testSrc, resSrc, name]);
}

// Invoked by generated code to define variables to used within
// subsequent tests
function define(expr) {
    tests.push(function() {
        with (defines) {
            eval("defines."+expr);
        }
    });
}

// Invoked by generated code to support bag comparison on this expected value
function bag(list) {
    var bag = eval(list).sort();
    var fun = function(other) {
        other = other.sort();
        return eq_test(bag, other);
    };
    fun.toString = function() {
        return "bag("+list+")";
    };
    return fun;
}

// Invoked by generated code to demonstrate expected error output
function err(err_name, err_msg, err_frames) {
    var err_frames = null; // TODO: test for frames
    var fun = function(other) {
        if (!(function() {
            if (!(other instanceof Error)) return false;
            if (err_name && !(other.name === err_name)) return false;

            // Strip out "offending object" from err message
            other.msg = other.msg.replace(/:\n([\r\n]|.)*/m, ".");
            other.msg = other.msg.replace(/\nFailed assertion([\r\n]|.)*/m, "");

            if (err_msg && !(other.msg === err_msg)) return false;
            if (err_frames && !(eq_test(other.frames, err_frames))) return false;
            return true;
        })()) {
            return false;
        }
        return true;
    }
    fun.isErr = true;
    fun.toString = function() {
        return err_name+"(\""+err_msg+"\")";
    };
    return fun;
}

function arrlen(length, eq_fun) {
    var fun = function(thing) {
        if (!thing.length || thing.length !== length) return false;
        return !eq_fun || thing.every(eq_fun);
    };
    fun.toString = function() {
        return "arrlen("+length+(eq_fun ? ", "+eq_fun.toString() : '')+")";
    };
    return fun;
}

function uuid() {
    var fun = function(thing) {
        return thing.match && thing.match(/[a-z0-9]{8}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{12}/);
    };
    fun.toString = function() {
        return "uuid()";
    };
    return fun;
}

function shard() {
    // Don't do anything in JS
}

function the_end(){ }

True = true;
False = false;
