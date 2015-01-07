var path = require('path');

// -- load rethinkdb from the proper location

var r = require(path.resolve(__dirname, '..', 'importRethinkDB.js')).r;


// --

// all argument numbers are +1 because the script is #1
var DRIVER_PORT = process.argv[2] || process.env.RDB_DRIVER_PORT
var DB_AND_TABLE_NAME = process.argv[3] || process.env.TEST_DB_AND_TABLE_NAME || 'no_table_specified'

var TRACE_ENABLED = process.env.VERBOSE || false;

// -- utilities --

failure_count = 0;

function printTestFailure(name, src, messages) {
    failure_count += 1;
    console.log.apply(console,["\nTEST FAILURE: "+name+"\nTEST BODY: "+src+"\n"].concat(messages).concat(["\n"]));
}

function clone(source) {
    result = {}
    for (var key in source) {
        result[key] = source[key];
    }
    return result;
}

function eq_test(expected, result, compOpts, partial) {
    TRACE("eq_test: " + JSON.stringify(expected) + " | == | " + JSON.stringify(result) + " | == | " + partial);
    if (expected instanceof Function) {
        return expected(result);
    } else if (result instanceof Function) {
        return result(expected);

    } else if (typeof expected !== typeof result) {
        return false;

    } else if (parseFloat(expected) === expected && parseFloat(result) === result) {
        if (compOpts && 'precision' in compOpts && parseFloat(compOpts['precision']) === compOpts['precision']) {
            return Math.abs(expected - result) <= compOpts['precision'];
        } else {
            return expected === result;
        }

    } else if (typeof expected === 'string') {
        expected = expected.replace(/\nFailed assertion([\r\n]|.)*/m, "");
        result = result.replace(/\nFailed assertion([\r\n]|.)*/m, "");
        return expected == result;

    } else if (Array.isArray(expected)) {
        if (partial) {
            // short circut on length
            if (expected.length > result.length) return false;

            // Recurse on each element of expected
            var resultIndex = 0;
            for (var expectedIndex in expected) {
                var foundIt = false;
                for (; resultIndex < result.length; resultIndex++) {
                    if (eq_test(expected[expectedIndex], result[resultIndex], compOpts)) {
                        foundIt = true;
                        break;
                    }
                }
                if (foundIt == false) {
                    return false;
                }
            }
        } else {
            // short circut on length
            if (expected.length !== result.length) return false;

            // Recurse on each element of expected
            for (var i = 0; i < expected.length; i++) {
                if (!eq_test(expected[i], result[i], compOpts)) return false;
            }
        }

        return true;

    } else if (expected instanceof Buffer) {
        if (expected.length !== result.length) return false;
        for (var i = 0; i < expected.length; i++) {
            if (expected[i] !== result[i]) return false;
        }
        return true;

    } else if (expected instanceof Object) {
        // short circut on keys
        if (!eq_test(Object.keys(expected).sort(), Object.keys(result).sort(), compOpts, partial)) return false;

        // Recurse on each property of object
        for (var key in expected) {
            if (expected.hasOwnProperty(key) && !eq_test(expected[key], result[key], compOpts)) return false;
        }
        return true;

    } else {
        // Primitive comparison
        return ((typeof expected === typeof result) && (expected === result)) || (isNaN(expected) && isNaN(result))
    }
}

function le_test(a, b){
    if (a instanceof Object && !Array.isArray(a)) {
        if (!(b instanceof Object && !Array.isArray(b))) {
            return false;
        }
        var ka = Object.keys(a).sort();
        var kb = Object.keys(a).sort();
        if (ka < kb) {
            return true;
        }
        if (ka > kb) {
            return false;
        }
        var ret;
        for (k in ka) {
            k = ka[k];
            var a_lt_b = le_test(a[k], b[k]);
            var b_lt_a = le_test(b[k], a[k]);

            if (a_lt_b ^ b_lt_a) {
                return a_lt_b;
            }
        }
        return true;
    } else if (b instanceof Object && !Array.isArray(b)) {
        return true;
    } else {
        return a <= b;
    }
}

// -- Curried output test functions --

// Equality comparison
function eq(exp, compOpts) {
    var fun = function(val) {
        if (!eq_test(val, exp, compOpts)) {
            return false;
        } else {
            return true;
        }
    };
    fun.toString = function() {
        return JSON.stringify(exp);
    };
    return fun;
}

function returnTrue() {
    var fun = function(val) {
        return True;
    }
    fun.toString = function() {
        return 'Always true';
    }
    return fun;
}

// Tests are stored in list until they can be sequentially evaluated
var tests = []

// Any variables defined by the tests are stored here
var defines = {}

function TRACE(){
    if (TRACE_ENABLED) {
        console.log.apply(console, ["TRACE"].concat([].splice.call(arguments, 0)));
    }
}

// `doExit` will later get set to something that performs cleanup
var doExit = process.exit
process.on('SIGINT', function(){ doExit(128+2); })
process.on(
    'unexpectedException',
    function(err) {
        console.log("Unexpected exception: " + String(err))
        console.log("Stack is: " + String(err.stack))
        doExit(1);
    }
)

// Connect first to cpp server
r.connect({port:DRIVER_PORT}, function(cpp_conn_err, cpp_conn) {

    if(cpp_conn_err){
        console.log("Failed to connect to server:", cpp_conn_err);
        process.exit(1);
    }

    // Pull a test off the queue and run it
    function runTest() { try {
        var testPair = tests.shift();
        if (testPair) {
            if (testPair instanceof Function) {
                TRACE("==== runTest == function");
                testPair(runTest, cpp_conn);
                return;
            } else {
                var src = testPair[0]
                var exp_val = testPair[1]
                var testName = testPair[2];
                var runopts = testPair[3];
                var testopts = testPair[4];
                TRACE("==== runTest == non-function: " + src)

                if (!runopts) {
                    runopts = {max_batch_rows: 3}
                } else {
                    for (var opt in runopts) {
                        runopts[opt] = eval(runopts[opt])
                    }
                    if (!("maxBatchRows" in runopts)) {
                        runopts.maxBatchRows = 3
                    }
                }
                if (!testopts) {
                    testopts = {}
                }
                compOpts = {}
                if ('precision' in testopts) {
                    compOpts['precision'] = testopts['precision']
                }

                // - convert expected value into a function for comparison
                try {
                    with (defines) {
                        var exp_fun = eval(exp_val);
                    }
                } catch (err) {
                    // Oops, this shouldn't have happened
                    console.log(testName);
                    console.log(exp_val);
                    throw err;
                }
                if (!exp_fun) exp_fun = returnTrue();
                if (!(exp_fun instanceof Function)) exp_fun = eq(exp_fun, compOpts);

                TRACE('expected value: ' + exp_fun.toString() + ' from ' + exp_val)

                // - build the test
                var test = null;
                try {
                    with (defines) {
                        TRACE("before eval");
                        test = eval(src);
                        TRACE("after eval");
                    }
                } catch(bld_err) {
                    TRACE("build error")
                    if (exp_fun.isErr) {
                        if (!exp_fun(bld_err)) {
                            printTestFailure(testName, src,
                                ["Error eval'ing test src not equal to expected err:\n\tERROR: ", bld_err, "\n\tExpected: ", exp_fun]);
                        }
                    } else {
                        printTestFailure(testName, src, ["Error eval'ing test src:\n\t", bld_err]);
                    }

                    // continue to next test
                    runTest();
                    return;
                }

                // Run test first on cpp server
                if (testopts && testopts['reql-query'] == false) {
                    TRACE("non reql-query result: " + test)
                    if (test instanceof Object && test.each) {
                        process_iterable(test, testopts);
                    } else {
                        afterArray(null, test, testopts);
                    }
                } else {
                    TRACE("processing query: " + test)
                    try {
                        var clone_runopts = runopts ? clone(runopts) : {};
                        var clone_testopts = testopts ? clone(testopts) : {};
                        with (defines) {
                            test.run(cpp_conn, clone_runopts, function(err, cursor) {run_callback(err, cursor, clone_testopts)} );
                        }

                    } catch(err) {
                        TRACE("querry error - " + err)
                        if (exp_fun.isErr) {
                            if (!exp_fun(err)) {
                                printTestFailure(testName, src, ["Error running test not equal to expected err:\n\tERROR: ", err, "\n\tEXPECTED: ", exp_val]);
                            }
                        } else {
                            printTestFailure(testName, src, ["Error running test:\n\t", err]);
                        }

                        // Continue to next test
                        runTest();
                        return;
                    }
                }

                function afterArray(cpp_err, cpp_res) { try {
                    TRACE("afterArray - src:" + src + ", err:" + cpp_err + ", result:" + JSON.stringify(cpp_res) + " exected function: " + exp_fun.toString());
                    if (cpp_err) {
                        if (exp_fun.isErr) {
                            if (!exp_fun(cpp_err)) {
                                printTestFailure(testName, src, ["Error running test on server not equal to expected err:", "\n\tERROR: ", JSON.stringify(cpp_err), "\n\tEXPECTED ", exp_val]);
                            }
                        } else {
                            var info;
                            if (cpp_err.msg) {
                                info = cpp_err.msg;
                            } else if (cpp_err.message) {
                                info = cpp_err.message;
                            } else {
                                info = JSON.stringify(cpp_res);
                            }

                            if (cpp_err.stack) {
                                info += "\n\nStack:\n" + cpp_err.stack.toString();
                            }
                            printTestFailure(testName, src, ["Error running test on server:", "\n\tERROR: ", info]);
                        }
                    } else if (!exp_fun(cpp_res)) {
                        printTestFailure(testName, src, ["CPP result is not equal to expected result:", "\n\tVALUE: ", JSON.stringify(cpp_res), "\n\tEXPECTED: ", exp_val]);
                    }

                    // Continue to next test. Tests are fully sequential
                    // so you can rely on previous queries results in
                    // subsequent tests.
                    runTest();
                    return;
                } catch(err) {
                    console.log("stack: " + String(err.stack))
                    unexpectedException("afterArray", testName, err);
                } }

                function process_iterable(feed, test_options) {
                    TRACE('process_iterable')
                    var accumulator = [];

                    feed.each(
                        function(err, _row) {
                            TRACE('process_iterable_internal')
                            if (err) {
                                console.log("stack: " + String(err.stack));
                                unexpectedException("run_callback", testName, err);
                            } else {
                                try {
                                    if (test_options && test_options.rowfilter) {
                                        filterFunction = new Function('input', test_options.rowfilter);
                                        accumulator.push(filterFunction(_row));
                                    } else {
                                        accumulator.push(_row);
                                    }
                                } catch(err) {
                                    console.log("stack: " + String(err.stack));
                                    unexpectedException("run_callback: <<" + test_options.filter + ">>", testName, err);
                                }
                            }
                        },
                        function () {
                            if (test_options && test_options.arrayfilter) {
                                arrayFunction = new Function('input', test_options.arrayfilter);
                                accumulator = arrayFunction(accumulator)
                            }
                            afterArray(null, accumulator, test_options);
                        }
                    );
                }

                function run_callback(cpp_err, cpp_res_cursor, test_options) { try {
                    TRACE("run_callback src:" + src + ", err:" + cpp_err + ", result:" + cpp_res_cursor);

                    if (test_options && test_options['variable']) {
                        defines[test_options['variable']] = cpp_res_cursor;
                    }

                    if (cpp_err) {
                        afterArray(cpp_err, null, test_options);
                    } else if (cpp_res_cursor instanceof Object && cpp_res_cursor.toArray) {
                        try {
                            cpp_res_cursor.toArray(function (err, result) { afterArray(err, result, test_options)} );
                        } catch(err) {
                            if (err instanceof r.Error.RqlDriverError) {
                                // probably a Feed
                                afterArray(null, null, test_options);
                            } else {
                                throw err;
                            }
                        }
                    } else {
                        afterArray(null, cpp_res_cursor, test_options);
                    }
                } catch(err) {
                    console.log("stack: " + String(err.stack))
                    unexpectedException("run_callback", testName, err);
                } }
            }
        } else {
            // We've hit the end of our test list
            if(failure_count != 0){
                console.log("Failed " + failure_count + " tests");
                doExit(1);
            } else {
                console.log("Passed all tests")
                doExit(0);
            }
        }
    } catch (err) {
        console.log("stack: " + String(err.stack))
        unexpectedException("runTest", testName, testPair[1], err);
    } }

    // Start the recursion though all the tests
    r.dbCreate('test').run(cpp_conn, runTest);
});

function unexpectedException(){
    console.log("Oops, this shouldn't have happened:");
    console.log.apply(console, arguments);
    doExit(1)
}

// Invoked by generated code to add test and expected result
// Really constructs list of tests to be sequentially evaluated
function test(testSrc, resSrc, name, runopts, testopts) {
    tests.push([testSrc, resSrc, name, runopts, testopts])
}

// Generated code must call either `setup_table()` or `check_no_table_specified()`
function setup_table(table_variable_name, table_name) {
    tests.push(function(next, cpp_conn) {
        try {
            doExit = function (exit_code) {
                console.log("cleaning up table...");
                // Unregister handler to prevent recursive insanity if something fails
                // while cleaning up
                doExit = process.exit
                if (DB_AND_TABLE_NAME === "no_table_specified") {
                    try {
                        r.db("test").tableDrop(table_name).run(cpp_conn, {}, function (err, res) {
                            if (err) {
                                unexpectedException("teardown_table", err);
                            }
                            if (res.tables_dropped != 1) {
                                unexpectedException("teardown_table", "table not dropped", res);
                            }
                            process.exit(exit_code);
                        });
                    } catch (err) {
                        console.log("stack: " + String(err.stack));
                        unexpectedException("teardown_table");
                    }
                } else {
                    var parts = DB_AND_TABLE_NAME.split(".");
                    try {
                        r.db(parts[0]).table(parts[1]).delete().run(cpp_conn, {}, function(err, res) {
                            if (err) {
                                unexpectedException("teardown_table", err);
                            }
                            if (res.errors !== 0) {
                                unexpectedException("teardown_table", "error when deleting", res);
                            }
                            try {
                                r.db(parts[0]).table(parts[1]).indexList().forEach(
                                        r.db(parts[0]).table(parts[1]).indexDrop(r.row))
                                    .run(cpp_conn, {}, function(err, res) {
                                        if (err) {
                                            unexpectedException("teardown_table", err);
                                        }
                                        if (res.errors !== undefined && res.errors !== 0) {
                                            unexpectedException("teardown_table", "error dropping indexes", res);
                                        }
                                        process.exit(exit_code);
                                });
                            } catch (err) {
                                console.log("stack: " + String(err.stack));
                                unexpectedException("teardown_table");
                            }
                        });
                    } catch (err) {
                        console.log("stack: " + String(err.stack));
                        unexpectedException("teardown_table");
                    }
                }
            }
            if (DB_AND_TABLE_NAME === "no_table_specified") {
                r.db("test").tableCreate(table_name).run(cpp_conn, {}, function (err, res) {
                    if (err) {
                        unexpectedException("setup_table", err);
                    }
                    if (res.tables_created != 1) {
                        unexpectedException("setup_table", "table not created", res);
                    }
                    defines[table_variable_name] = r.db("test").table(table_name);
                    next();
                });
            } else {
                var parts = DB_AND_TABLE_NAME.split(".");
                defines[table_variable_name] = r.db(parts[0]).table(parts[1]);
                next();
            }
        } catch (err) {
            console.log("stack: " + String(err.stack));
            unexpectedException("setup_table");
        }
    });
}

function check_no_table_specified() {
    if (DB_AND_TABLE_NAME !== "no_table_specified") {
        unexpectedException("This test isn't meant to be run against a specific table")
    }
}

// Invoked by generated code to define variables to used within
// subsequent tests
function define(expr) {
    tests.push(function(next, cpp_conn) {
        with (defines) {
            eval("defines."+expr);
        }
        next();
    });
}

// Invoked by generated code to support bag comparison on this expected value
function bag(expected, compOpts, partial) {
    var bag = eval(expected).sort(le_test);
    var fun = function(other) {
        other = other.sort(le_test);
        return eq_test(bag, other, compOpts, true);
    };
    fun.toString = function() {
        return "bag(" + expected + ")";
    };
}

function partial(expected, compOpts) {
    if (Array.isArray(expected)) {
        return bag(expected, compOpts, true);
    } else if (expected instanceof Object) {
        var fun = function(result) {
            return eq_test(expected, result, compOpts, true);
        };
        fun.toString = function() {
            return "partial dict(" + expected + ")";
        };
        return fun;
    } else {
        unexpectedException("partial can only handle Arrays and Objects, got: " + typeof(expected));
    }
}

// Invoked by generated code to demonstrate expected error output
function err(err_name, err_msg, err_frames) {
    return err_predicate(
        err_name, function(msg) { return (!err_msg || (err_msg === msg)); },
        err_frames, err_name+"(\""+err_msg+"\")");
}

function err_regex(err_name, err_pat, err_frames) {
    return err_predicate(
        err_name, function(msg) { return (!err_pat || new RegExp(err_pat).test(msg)); },
        err_frames, err_name + "(\""+err_pat+"\")");
}

function err_predicate(err_name, err_pred, err_frames, desc) {
    var err_frames = null; // TODO: test for frames
    var fun = function(other) {
        if (!(other instanceof Error)) return false;
        if (err_name && !(other.name === err_name)) return false;

        // Strip out "offending object" from err message
        other.msg = other.msg.replace(/:\n([\r\n]|.)*/m, ".");
        other.msg = other.msg.replace(/\nFailed assertion([\r\n]|.)*/m, "");

        if (!err_pred(other.msg)) return false;
        if (err_frames && !(eq_test(other.frames, err_frames))) return false;
        return true;
    }
    fun.isErr = true;
    fun.toString = function() {
        return desc;
    };
    return fun;
}

function builtin_err(err_name, err_msg) {
    var fun = function(other) {
        if (!(other.name === err_name)) return false;
        if (!(other.message === err_msg)) return false;
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
