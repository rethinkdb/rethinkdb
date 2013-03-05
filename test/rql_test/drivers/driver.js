var r = require('./../../../drivers/javascript2/build/rethinkdb.js');

var JSPORT = process.argv[2]
var CPPPORT = process.argv[3]

// -- utilities --

function printTestFailure(name, src, message) {
    console.log("\nTEST FAILURE: "+name+"\nTEST BODY: "+src+"\n"+message+"\n");
}

function eq_test(one, two) {
    if (one instanceof Array) {

        if (!(two instanceof Array)) return false;
        
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

// Connect first to cpp server
r.connect({port:CPPPORT}, function(cpp_conn_err, cpp_conn) {
    
    // Now connect to js server
    //r.connect({port:JSPORT}, function(js_conn_err, js_conn) {

        // Pull a test off the queue and run it
        function runTest() {
            var testPair = tests.shift();
            if (testPair) {
                if (testPair instanceof Function) {
                    testPair();
                    runTest();
                    return;
                } else {

                    testName = testPair[2];

                    var exp_fun = eval(testPair[1]);
                    if (!exp_fun)
                        exp_fun = function() { return true; };

                    if (!(exp_fun instanceof Function))
                        exp_fun = eq(exp_fun);

                    var src = testPair[0]
                    try {
                        with (defines) {
                            test = eval(src);
                        }
                    } catch(bld_err) {
                        if (exp_fun.isErr) {
                            if (!exp_fun(bld_err)) {
                                printTestFailure(testName, src,
                                    "Error eval'ing test src not equal to expected err:\n\tERROR: "+
                                        bld_err.toString()+"\n\tExpected: "+exp_fun.toString());
                            }
                        } else {
                            printTestFailure(testName, src, "Error eval'ing test src:\n\t"+bld_err.toString());
                        }

                        // continue to next test
                        runTest();
                        return;
                    }

                    // Run test first on cpp server
                    try {
                        test.run(cpp_conn, cpp_cont);
                    } catch(err) {
                        if (exp_fun.isErr) {
                            if (!exp_fun(err)) {
                                printTestFailure(testName, src,
                                    "Error running test not equal to expected err:\n\tERROR: "+
                                        err.toString()+"\n\tEXPECTED: "+exp_fun.toString());
                            }
                        } else {
                            printTestFailure(testName, src, "Error running test:\n\t"+err.toString());
                        }
                        
                        // Continue to next test
                        runTest();
                        return;
                    }

                    function cpp_cont(cpp_err, cpp_res) {

                        // Convert to array if it's a cursor
                        if (cpp_res instanceof Object && cpp_res.toArray) {
                            cpp_res.toArray(afterArray);
                        } else {
                            afterArray(null, cpp_res);
                        }
                        
                        function afterArray(arr_err, cpp_res) {

                            // Now run test on js server
                            //test.run(js_conn, js_cont);

                            //function js_cont(js_err, js_res) {

                                /*
                                if (js_res instanceof Object && js_res.toArray) {
                                    js_res.toArray(afterArray2);
                                } else {
                                    afterArray2(null, js_res);
                                }
                                */
                                afterArray2(null, null);

                                // Again, convert to array
                                function afterArray2(arr_err, js_res) {

                                    if (cpp_err) {
                                        if (exp_fun.isErr && !exp_fun(cpp_err)) {
                                            printTestFailure(testName, src,
                                                "Error running test on CPP server not equal to expected err:"+
                                                "\n\tERROR: "+cpp_err.toString()+
                                                "\n\tEXPECTED "+exp_fun.toString());
                                        }
                                    } else if (!exp_fun(cpp_res)) {
                                        printTestFailure(testName, src,
                                            "CPP result is not equal to expected result:"+
                                            "\n\tVALUE: "+cpp_res.toString()+"\n\tEXPECTED: "+exp_fun.toString());
                                    }

                                    /*
                                    if (js_err) {
                                        if (exp_fun.isErr && !exp_fun(js_err)) {
                                            printTestFailure(testName, src,
                                                "Error running test on JS server not equal to expected err:"+
                                                "\n\tERROR: "+js_err.toString()+
                                                "\n\tEXPECTED "+exp_fun.toString());
                                        }
                                    } else if (!exp_fun(js_res)) {
                                        printTestFailure(testName, src,
                                            "JS result is not equal to expected result:"+
                                            "\n\tVALUE: "+js_res.toString()+"\n\tEXPECTED: "+exp_fun.toString());
                                    }
                                    */

                                    // Continue to next test. Tests are fully sequential
                                    // so you can rely on previous queries results in
                                    // subsequent tests.
                                    runTest();
                                    return;
                                }
                            //}
                        }
                    }
                }
            } else {
                // We've hit the end of our test list
                // closing the connection will allow the
                // event loop to quit naturally
                cpp_conn.close();
                //js_conn.close();
            }
        }

        // Start the recursion though all the tests
        runTest();
    //});
});

// Invoked by generated code to add test and expected result
// Really constructs list of tests to be sequentially evaluated
function test(testSrc, resSrc, name) {
    tests.push([testSrc, resSrc, name])
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
    var err_frames = null; // Don't test frames for now, at least not until the C++ is done with them
    var fun = function(other) {
        if (!(function() {
            if (!(other instanceof Error)) return false;
            if (err_name && !(other.name === err_name)) return false;

            // Strip out "offending object" from err message
            other.msg = other.msg.replace(/:(\n|.)*/m, ".");

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

True = true;
False = false;
