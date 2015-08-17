var Path = require('path');
var Promise = require(Path.resolve(__dirname, '..', '..', '..', 'build', 'packages', 'js', 'node_modules', 'bluebird'));

// -- load rethinkdb from the proper location

var r = require(Path.resolve(__dirname, '..', 'importRethinkDB.js')).r;

// -- global variables

// Tests are stored in list until they can be sequentially evaluated
var tests = [r.dbCreate('test')]

var failure_count = 0;
var tests_run = 0;

var start_time = Date.now()

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

function eq_test(expected, result, compOpts, partial) {
    TRACE("eq_test: " + stringValue(expected) + " | == | " + stringValue(result) + " || partial: " + (partial == true));
    
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
        return ((typeof expected === typeof result) && (expected === result)) || (Number.isNaN(Number(expected)) && Number.isNaN(Number(result)))
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
    	var prefix = "TRACE " + ((Date.now() - start_time) / 1000).toFixed(2) + ": \t";
        console.log(prefix.concat([].splice.call(arguments, 0)));
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

    // Start the chain of tests
    runTest();
});

// Pull a test off the queue and run it
function runTest() {
    try {
        var test = tests.shift();
        if (test) {
            if (test instanceof Function) {
                // -- function such as setup_table
                TRACE("==== runTest ==== function: " + test.name);
                test(test);
                return;
            
            } else {
                // -- regular test
                TRACE("==== runTest ==== test: " + stringValue(test.src))
                
                // - process/default runopts
                
                if (!test.runopts) {
                    test.runopts = { maxBatchRows: 3 }
                } else {
                    for (var opt in test.runopts) {
	                    try {
	                        test.runopts[opt] = eval(test.runopts[opt])
	                    } catch (err) {
		                    test.runopts[opt] = test.runopts[opt]
		                }
                    }
                    if (!("maxBatchRows" in test.runopts)) {
                        test.runopts.maxBatchRows = 3
                    }
                }
                // the javascript driver has all of the runopts in cammel case vs. snake form
                for (var opt in test.runopts) {
	                if (typeof opt == 'string' || opt instanceof String) {
		                var newOpt = opt.replace(/([a-z]+)_([a-z]+)/, function(match, pre, post) { return pre + post.charAt(0).toUpperCase() + post.substr(1); });
						if (newOpt != opt) {
							test.runopts[newOpt] = test.runopts[opt];
							delete test.runopts[opt];
						}
	                }
	            }
                TRACE("runopts: " + JSON.stringify(test.runopts))
                
                // - process/default testopts
                
                if (!test.testopts) {
                    test.testopts = {}
                }
                compOpts = {}
                if ('precision' in test.testopts) {
                    compOpts['precision'] = test.testopts['precision']
                }
                
                // - convert expected value into a function for comparison
                var exp_fun = null;
                try {
                    with (defines) {
                        exp_fun = eval(test.expectedSrc);
                    }
                } catch (err) {
                    // Oops, this shouldn't have happened
                    console.log(test.name);
                    console.log(test.expectedSrc);
                    throw err;
                }
                if (!exp_fun) exp_fun = returnTrue();
                if (!(exp_fun instanceof Function)) exp_fun = eq(exp_fun, compOpts);
                
                test.exp_fun = exp_fun;
                
                TRACE('expected value: ' + stringValue(test.exp_fun) + ' from ' + stringValue(test.expectedSrc))
                
                // - evaluate the test
                
                var result = null;
                try {
                    with (defines) {
                        TRACE("before eval");
                        result = eval(test.src);
                        TRACE("after eval");
                        
                        // - allow autoRunTest functions to take over
                        
                        if (result instanceof Function && result.autoRunTest) {
                            TRACE("processing auto-run test: " + result)
                            result(test);
                            return; // the auto-run should take over
                        }
                        
                        // - 
                        
                        if (result instanceof r.table('').__proto__.__proto__.__proto__.constructor) {
                            TRACE("processing reql query: " + result + ', runopts: ' + stringValue(test.runopts))
                            with (defines) {
                                result.run(reqlConn, test.runopts, function(err, value) { processResult(err,  value, test) } );
                                return;
                            }
                        } else if (result && result.hasOwnProperty('_settledValue')) {
                            TRACE("processing promise: " + result)
                            result.then(function(result) {
                                processResult(null, result, test); // will go on to next test
                            }).error(function(err) {
                                processResult(err, null, test); // will go on to next test
                            });
                            return;
                        } else {
                            resultText = result
                            try {
                                resultText = result.toString();
                            } catch (err) {}
                            TRACE("non reql-query result: " + resultText)
                            processResult(null, result, test); // will go on to next test
                            return;
                        }
                    }
                } catch (result) {
                    TRACE("querry error - " + result.toString())
                    TRACE("stack: " + String(result.stack));
                    processResult(result, null, test); // will go on to next test
                    return;
                }
            
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
        unexpectedException("runTest", test.name, test, err);
    }
}

function processResult(err, result, test) {
    // prepare the result to be compared (e.g.: collect feeds and cursor results)
    TRACE('processResult: ' + result + ', err: ' + err + ', testopts: ' +  stringValue(test.testopts))
    var accumulator = [];
    
    try {
        // - if an error go straight to compare
        
        if (err) {
            TRACE('processResult error');
            compareResult(err, null, test);
        }
        
        // - store variable if called for
        
        else if (test.testopts && test.testopts.variable) {
            TRACE('processResult storing variable');
            defines[test.testopts.variable] = result;
            compareResult(null, result, test);
        }
        
        // - pull out feeds and cursors to arrays
        
        else if (result instanceof Object && result.each) {
            if (!Number.isNaN(Number(test.testopts.result_limit)) && test.testopts.result_limit > 0) {
                TRACE('processResult_limitedIter collecting ' + test.testopts.result_limit + ' item(s) from cursor');
                
                handleError = function processResult_error(err) {
	                TRACE('processResult_limitedIter error: ' + err)
                    compareResult(err, accumulator, test);
                };
                
                limitedProcessor = function processResult_limitedIter(row) {
                    accumulator.push(row);
                    if (accumulator.length >= test.testopts.result_limit) {
                        // - limit reached
                        TRACE('processResult_limitedIter final, items: ' + accumulator.length);
                        compareResult(null, accumulator, test);
                    } else {
                        TRACE('processResult_limitedIter next, collected: ' + accumulator.length + ', item: ' + stringValue(row));
                        result.next().then(limitedProcessor).error(handleError);
                    }
                    
                };
                
                // start accumulator loop
                result.next().then(limitedProcessor).error(handleError);
                
            } else {
                TRACE('processResult collecting full cursor');
                result.each(
                    function processResult_iter(err, row) {
                        if (err) {
	                        TRACE('processResult_iter err: ' + err);
	                        compareResult(err, accumulator, test);
                        } else {
	                        TRACE('processResult_iter value');
                            try {
                                if (test.testopts && test.testopts.rowfilter) {
                                    filterFunction = new Function('input', test.testopts.rowfilter);
                                    row = filterFunction(row);
                                    if (typeof row != 'undefined') {
                                        accumulator.push(row);
                                    }
                                } else {
                                    accumulator.push(row);
                                }
                            } catch(err) {
                                console.log("stack: " + String(err.stack));
                                unexpectedException("processResult_iter <<" + test.testopts.filter + ">>", test.name, err);
                            }
                        }
                    },
                    function processResult_iterFinal() {
                        TRACE('processResult_final' + test);
                        if (test.testopts && test.testopts.arrayfilter) {
                            arrayFunction = new Function('input', test.testopts.arrayfilter);
                            accumulator = arrayFunction(accumulator);
                        }
                        compareResult(null, accumulator, test);
                    }
                );
            }
        
        // - otherwise go to compare
        
        } else {
            compareResult(null, result, test);
        }
    } catch(err) {
        console.log("stack: " + String(err.stack))
        unexpectedException("processResult", test.name, err);
    }
}

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

function compareResult(error, value, test) {
    try {
        expextedText = null;
        TRACE("compareResult - err:" + stringValue(error) + ", result:" + stringValue(value) + " expected function: " + test.exp_fun.toString());
        if (error) {
            if (test.exp_fun.isErr) {
                if (!test.exp_fun(error)) {
                    printTestFailure(test.name, test.src, test.expectedSrc, stringValue(error));
                }
            } else {
                printTestFailure(test.name, test.src, test.expectedSrc, stringValue(error));
            }
        } else if (!test.exp_fun(value)) {
            printTestFailure(test.name, test.src, test.expectedSrc, stringValue(value));
        }
    
        runTest(); // Continue to next test.
    } catch(err) {
        console.log("stack: " + String(err.stack))
        unexpectedException("compareResult", test.name, err);
    }
}

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
function test(testSrc, expectedSrc, name, runopts, testopts) {
    tests.push({
        'src':testSrc,
        'expectedSrc':expectedSrc,
        'name':name,
        'runopts':runopts,
        'testopts':testopts
    })
}

function setup_table(table_variable_name, table_name, db_name) {
    tests.push(function setup_table_inner(test) {
        try {
            if (required_external_tables.length > 0) {
                // use an external table
                
                table = required_external_tables.pop();
                defines[table_variable_name] = r.db(table[0]).table(table[1]);
                tables_to_cleanup.push([table[0], table[1]])
                
                // check that the table exists
                r.db(table[0]).table(table[1]).info().run(reqlConn, function(err, res) {
                    if (err) {
                        unexpectedException("External table " + table[0] + "." + table[1] + " did not exist")
                    } else {
                        runTest();
                    }
                });
            } else {
                // create the table as provided
                
                r.db(db_name).tableCreate(table_name).run(reqlConn, {}, function (err, res) {
                    if (err) {
                        unexpectedException("setup_table", err);
                    }
                    if (res.tables_created != 1) {
                        unexpectedException("setup_table", "table not created", res);
                    }
                    defines[table_variable_name] = r.db("test").table(table_name);
                    tables_to_delete.push([db_name, table_name])
                    runTest();
                });
            }
        } catch (err) {
            console.log("stack: " + String(err.stack));
            unexpectedException("setup_table");
        }
    });
}

// check that all of the requested tables have been setup
function setup_table_check() {
    tests.push(function setup_table_check_inner() {
        if (required_external_tables.length > 0) {
            tableNames = []
            for (i in required_external_tables) {
                tableNames.push(required_external_tables[0] + '.' + required_external_tables[1])
            }
            throw 'Unused external tables, that is probably not supported by this test: ' + tableNames.join(', ');
        }
        runTest();
    });
}

// Invoked by generated code to fetch from a cursor
function fetch(cursor, limit) {
    fun = function fetch_inner(test) {
        TRACE("fetch_inner test: " + stringValue(test))
        try {
            if (limit) {
                limit = parseInt(limit);
                if (Number.isNaN(limit)) {
                    unexpectedException("The limit value of fetch must be null or a number ")
                }
            }
            if (!test.testopts) {
                test.testopts = {};
            }
            test.testopts.result_limit = limit;
            TRACE('fetching ' + (limit || "all") + ' items')
            processResult(null, cursor, test)
        } catch(err) {
            console.log("stack: " + String(err.stack))
            unexpectedException("processResult", test.name, err);
        }
    };
    fun.toString = function() {
        return 'fetch_inner() limit = ' + limit;
    };
    fun.autoRunTest = true;
    return fun;
}

// allows for a bit of time to go by
function wait(seconds) {
    fun = function wait_inner(test) {
        TRACE("wait_inner test: " + stringValue(test))
        setTimeout(function () { runTest() }, seconds * 1000);
    };
    fun.toString = function() {
        return 'wait_inner() seconds = ' + seconds;
    };
    fun.autoRunTest = true;
    return fun;
}

// Invoked by generated code to define variables to used within
// subsequent tests
function define(expr, variable) {
    tests.push(function define_inner(test) {
        TRACE('setting define: ' + variable + ' = '  + expr);
        with (defines) {
            eval("defines." + variable + " = " + expr);
        }
        runTest();
    });
}

// Invoked by generated code to support bag comparison on this expected value
function bag(expected, compOpts, partial) {
    
    // note: This only works for dicts, arrays, numbers, undefines, and strings. Anything else might as well be random.
    
    function sortFuncName(obj) {
        if (obj === undefined) {
            return 'undefined';
        } else if (Number.isNaN(Number(obj)) === false && !(obj instanceof Array)) {
            return 'Number';
        } else if (obj.name) {
            return obj.name;
        } else if (obj.constructor && obj.constructor.name) {
            return obj.constructor.name;
        } else {
            return stringValue(obj);
        }
    }
    
    function sortFunc(left, right) {
        TRACE('sorting: ' + stringValue(left) + ' vs.: ' + stringValue(right) + ' ' + sortFuncName(left))
        // -- compare object types, sorting by name
        
        var result = sortFuncName(left).localeCompare(sortFuncName(right))
        if (result != 0) { return result; }
        
        // --- null/undefined
        
        if (left === undefined) {
            if (right === undefined) {
                return 0;
            } else {
                return -1;
            }
        
        // -- array
        
        } else if (left instanceof Array) {
            TRACE('array')
            var i = 0;
            for (i = 0; i < left.length; i++) {
                if (i >= right.length) {
                    return -1;
                }
                TRACE('item ' + i + ' ' + left[i] + ' ' +  right[i])
                result = sortFunc(left[i], right[i]);
                if (result != 0) { return result; }
            }
            if (right.length > i) {
                return 1;
            }
        
        // -- dict
        
        } else if (left instanceof Object) {
            leftKeys = Object.keys(left).sort(sortFunc);
            rightKeys = Object.keys(right).sort(sortFunc);
            
            // if they don't have the same keys, sort by that
            result = sortFunc(leftKeys, rightKeys);
            if (result != 0) { return result; }
            
            for (var i in leftKeys) {
                key = leftKeys[i];
                result = sortFunc(left[key], right[key]);
                if (result != 0) { return result; }
            }
        
        // -- number
        
        } else if (Number.isNaN(Number(left)) === false && Number.isNaN(Number(right)) === false) {
            left = Number(left);
            right = Number(right);
            return left == right ? 0 : left > right;
        
        // -- string
        
        } else {
            return left.localeCompare(right);
        }
        return 0;
    }

    var bag = eval(expected).sort(sortFunc);
    var fun = function bag_inner(other) {
        other = other.sort(sortFunc);
        TRACE("bag: " + stringValue(bag) + " other: " + stringValue(other))
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
