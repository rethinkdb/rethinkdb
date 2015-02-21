var Path = require('path');
var Promise = require(Path.resolve(__dirname, '..', '..', '..', 'build', 'packages', 'js', 'node_modules', 'bluebird'));

// -- load rethinkdb from the proper location

var r = require(Path.resolve(__dirname, '..', 'importRethinkDB.js')).r;

// -- global variables

// Tests are stored in list until they can be sequentially evaluated
var tests = []

var failure_count = 0;
var tests_run = 0;

// Provides a context for variables
var defines = {}

var reqlConn = null; // set as testing begins

var tables_to_cleanup = [] // pre-existing tables
var tables_to_delete = [] // created by this script

// -- input validation

// all argument numbers are +1 because the script is #1
var DRIVER_PORT = process.argv[2] || process.env.RDB_DRIVER_PORT;
var required_external_tables = [];
if (process.argv[3] || process.env.TEST_DB_AND_TABLE_NAME) {
    rawValues = (process.argv[3] || process.env.TEST_DB_AND_TABLE_NAME).split(',');
    for (i in rawValues) {
        rawValue = rawValues[i].trim();
        if (rawValue == '') {
            continue;
        }
        splitValue = rawValue.split('.')
        if (splitValue.length == 1) {
            required_external_tables.push(['test', splitValue[0]]);
        } else if (splitValue.length == 2) {
            required_external_tables.push([splitValue[0], splitValue[1]]);
        } else {
            throw 'Unusable value for external tables:' + stringValue(rawValue);
        }
    }
}
required_external_tables.reverse() // so pop'ing them gets the right order

var TRACE_ENABLED = process.env.VERBOSE || false;

// -- utilities --

function printTestFailure(name, src, expected, result) {
    failure_count += 1;
    console.log("TEST FAILURE:  " + name + "\n" +
    			"     SOURCE:   " + src + "\n" +
    			"     EXPECTED: " + expected + "\n" +
    			"     RESULT:   " + result + "\n");
}

function clone(source) {
    result = {}
    for (var key in source) {
        result[key] = source[key];
    }
    return result;
}

function eq_test(expected, result, compOpts, partial) {
    TRACE("eq_test: " + stringValue(expected) + " | == | " + stringValue(result) + " | == | " + partial);
    
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
            // short circuit on length
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
            // short circuit on length
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
        // short circuit on keys
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
        var kb = Object.keys(b).sort();
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
    var fun = function eq_inner (val) {
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
    var fun = function returnTrue_inner (val) {
        return True;
    }
    fun.toString = function() {
        return 'Always true';
    }
    return fun;
}

function TRACE(){
    if (TRACE_ENABLED) {
        console.log.apply(console, ["TRACE"].concat([].splice.call(arguments, 0)));
    }
}

// -- Setup atexit handler for cleanup

function atexitCleanup(exitCode) {
    
    if (!reqlConn) {
        console.warn('Unable to clean up as there is no open ReQL connection')
    } else {
        console.log('Cleaning up')
        
        promisesToKeep = [];
        
        // - cleanup tables
        
        cleanTable = function(dbName, tableName, fullName) {
            return r.db(dbName).tableList().count(tableName).eq(1).run(reqlConn, function(err, value) {
                if (err) { throw 'In cleanup there was no table "' + fullName + '" to clean'; }
            }).then(function() {
                return r.db(dbName).table(tableName).indexList().forEach(r.db(dbName).table(tableName).indexDrop(r.row)).run(reqlConn, function(err, value) {
                    if (err) {
                        throw 'In cleanup failure to remove indexes from table "' + fullName + '": ' + err;
                    }
                });
            }).then(function() {
                return r.db(dbName).table(tableName).delete().run(reqlConn, function(err, value) {
                    if (err) {
                        throw 'In cleanup failure to remove data from table "' + fullName + '": ' + err;
                    }
                });
            }).error(function (e) {
                if (exitCode == 0) { exitCode = 1; }
                console.error(e);
            })
        }
        
        for (i in tables_to_cleanup) {
            dbName = tables_to_cleanup[i][0];
            tableName = tables_to_cleanup[i][1];
            fullName = dbName + '.' + tableName;
            
            promisesToKeep.push(cleanTable(dbName, tableName, fullName));
        }
        
        // - remove tables
        
        deleteTable = function(dbName, tableName, fullName) {
            return r.db(dbName).tableDrop(tableName).run(reqlConn, function(err, value) {
                if (err) {
                    if (exitCode == 0) { exitCode = 1; }
                    console.error('In cleanup there was no table "' + fullName + '" to delete: ' + err);
                }
            });
        }
        
        for (i in tables_to_delete) {
            dbName = tables_to_delete[i][0];
            tableName = tables_to_delete[i][1];
            fullName = dbName + '.' + tableName;
            
            promisesToKeep.push(deleteTable(dbName, tableName, fullName));
        }
        
        if(promisesToKeep.length > 0) {
            Promise.all(promisesToKeep).then(function () {
                process.exit(exitCode);
            });
        } else {
            process.exit(exitCode);
        }
    }
}

process.on('SIGINT', function(){ atexitCleanup(128+2); })
process.on('beforeExit', function(){ atexitCleanup(failure_count != 0); })
process.on('unexpectedException',
    function(err) {
        console.log("Unexpected exception: " + String(err))
        console.log("Stack is: " + String(err.stack))
        atexitCleanup(1);
    }
)

// Connect first to cpp server
r.connect({port:DRIVER_PORT}, function(error, conn) {

    if(error){
        console.error("Failed to connect to server:", error);
        process.exit(1);
    }
    reqlConn = conn;

    // Pull a test off the queue and run it
    function runTest() { try {
        var testPair = tests.shift();
        if (testPair) {
            if (testPair instanceof Function) {
                TRACE("==== runTest == function");
                testPair(runTest, reqlConn);
                return;
            
            } else {
                var src = testPair[0]
                var exp_val = testPair[1]
                var testName = testPair[2];
                var runopts = testPair[3];
                var testopts = testPair[4];
                TRACE("==== runTest == non-function: " + src)

                if (!runopts) {
                    runopts = {maxBatchRows: 3}
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
                var exp_fun = null;
                try {
                    with (defines) {
                       exp_fun = eval(exp_val);
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
                            printTestFailure(testName, src, exp_val, bld_err);
                        }
                    } else {
                        printTestFailure(testName, src, exp_val, bld_err);
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
                            test.run(reqlConn, clone_runopts, function(err, cursor) {run_callback(err, cursor, clone_testopts)} );
                        }

                    } catch(err) {
                        TRACE("querry error - " + err)
                        if (exp_fun.isErr) {
                            if (!exp_fun(err)) {
                                printTestFailure(testName, src, exp_val, err);
                            }
                        } else {
                            printTestFailure(testName, src, exp_val, err);
                        }

                        // Continue to next test
                        runTest();
                        return;
                    }
                }

                function afterArray(cpp_err, cpp_res) { try {
                    TRACE("afterArray - src:" + src + ", err:" + cpp_err + ", result:" + JSON.stringify(cpp_res) + " expected function: " + exp_fun.toString());
                    if (cpp_err) {
                        if (exp_fun.isErr) {
                            if (!exp_fun(cpp_err)) {
                                printTestFailure(testName, src, exp_val, stringValue(cpp_err));
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
                            printTestFailure(testName, src, exp_val, info);
                        }
                    } else if (!exp_fun(cpp_res)) {
                        printTestFailure(testName, src, exp_val, stringValue(cpp_res));
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
                atexitCleanup(1);
            } else {
                console.log("Passed all tests")
                atexitCleanup(0);
            }
        }
    } catch (err) {
        console.log("stack: " + String(err.stack))
        unexpectedException("runTest", testName, testPair[1], err);
    } }

    // Start the recursion though all the tests
    r.dbCreate('test').run(reqlConn, runTest);
});

function stringValue(value) {
    returnValue = '<< unknown >>';
    if (value instanceof Error) {
	    var errStr = value.name ? value.name + ": " : '';
	    
	    if (value.msg) {
			errStr += value.msg;
		} else if (value.message) {
			errStr += value.message;
		} else {
			errStr += stringValue(value);
		}
		
		if (value.stack) {
			errStr += "\nStack:\n" + value.stack.toString();
		}
	    return errStr;
	} else if (value && value.name) {
        returnValue = value.name;
    } else {
		try {
			returnValue = JSON.stringify(value);
		} catch (err) {
			try {
				returnValue = value.toString();
			} catch (err) {}
		}
    }
    return returnValue;
}

function unexpectedException(){
    console.log("Oops, this shouldn't have happened:");
    console.log.apply(console, arguments);
    atexitCleanup(1)
}

// Invoked by generated code to add test and expected result
// Really constructs list of tests to be sequentially evaluated
function test(testSrc, resSrc, name, runopts, testopts) {
    tests.push([testSrc, resSrc, name, runopts, testopts])
}

function setup_table(table_variable_name, table_name, db_name) {
    tests.push(function(next, cpp_conn) {
        try {
            if (required_external_tables.length > 0) {
                // use an external table
                
                table = required_external_tables.pop();
                defines[table_variable_name] = r.db(table[0]).table(table[1]);
                tables_to_cleanup.push([table[0], table[1]])
                next();
            } else {
                // create the table as provided
                
                r.db(db_name).tableCreate(table_name).run(cpp_conn, {}, function (err, res) {
                    if (err) {
                        unexpectedException("setup_table", err);
                    }
                    if (res.tables_created != 1) {
                        unexpectedException("setup_table", "table not created", res);
                    }
                    defines[table_variable_name] = r.db("test").table(table_name);
                    tables_to_delete.push([db_name, table_name])
                    next();
                });
            }
        } catch (err) {
            console.log("stack: " + String(err.stack));
            unexpectedException("setup_table");
        }
    });
}

// Invoked by generated code to define variables to used within
// subsequent tests
function define(expr) {
    tests.push(function define_inner (next, cpp_conn) {
        with (defines) {
            eval("defines."+expr);
        }
        next();
    });
}

// Invoked by generated code to support bag comparison on this expected value
function bag(expected, compOpts, partial) {
    var bag = eval(expected).sort(le_test);
    var fun = function bag_inner(other) {
        other = other.sort(le_test);
        return eq_test(bag, other, compOpts, true);
    };
    fun.toString = function() {
        return "bag(" + expected + ")";
    };
    return fun;
}

// Invoked by generated code to ensure at least these contents are in the result
function partial(expected, compOpts) {
    if (Array.isArray(expected)) {
        return bag(expected, compOpts, true);
    } else if (expected instanceof Object) {
        var fun = function partial_inner(result) {
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
        err_name,
        function(msg) { return (!err_msg || (err_msg === msg)); },
        err_frames || [],
        err_name+"(\""+err_msg+"\")"
    );
}

function err_regex(err_name, err_pat, err_frames) {
    return err_predicate(
        err_name,
        function(msg) { return (!err_pat || new RegExp(err_pat).test(msg)); },
        err_frames,
        err_name + "(\""+err_pat+"\")"
    );
}
errRegex = err_regex

function err_predicate(err_name, err_pred, err_frames, desc) {
    var err_frames = null; // TODO: test for frames
    var fun = function err_predicate_return (other) {
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
    var fun = function builtin_err_test (other) {
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
    var fun = function arrlen_test (thing) {
        if (!thing.length || thing.length !== length) return false;
        return !eq_fun || thing.every(eq_fun);
    };
    fun.toString = function() {
        return "arrlen("+length+(eq_fun ? ", "+eq_fun.toString() : '')+")";
    };
    return fun;
}

function uuid() {
    var fun = function uuid_test (thing) {
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

// REPLACE WITH TABLE CREATION LINES

if (required_external_tables.length > 0) {
    tableNames = []
    for (i in required_external_tables) {
        tableNames.push(required_external_tables[0] + '.' + required_external_tables[1])
    }
    throw 'Unused external tables, that is probably not supported by this test: ' + tableNames.join(', ');
}
