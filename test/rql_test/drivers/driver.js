var r = require('./../../../drivers/javascript2/build/rethinkdb.js');

var CPPPORT = 28016
var JSPORT = 28017

// Tests are stored in list until they can be sequentially evaluated
var tests = []

// Connect first to cpp server
r.connect({port:CPPPORT}, function(err, cpp_conn) {
	
	// Now connect to js server
	r.connect({port:JSPORT}, function(err, js_conn) {

		// Pull a test off the queue and run it
		function runTest() {
			var testPair = tests.shift();
			if (testPair) {
				try {
					test = eval(testPair[0]);
				} catch(err) {
					throw new Error("Query construction error")
				}
				expected = eval(testPair[1]);

				// Run test first on cpp server
				test.run(cpp_conn, function(cpp_err, cpp_res) {
					
					// Now run test on js server
					test.run(js_conn, function(js_err, js_res) {
						
						// First, are the two results equal to each other?
						if (cpp_res != js_res) {
							throw new Error("Cpp and JS results not equal");
						}

						// Now compare to expected result
						if (cpp_res !== expected) {
							throw new Error("Cpp result not expected");
						}

						if (js_res !== expected) {
							throw new Error("Js result not expected");
						}

						// Continue to next test. Tests are fully sequential
						// so you can rely on previous queries results in
						// subsequent tests.
						runTest();
					});
				});
			} else {
				// We've hit the end of our test list
				// closing the connection will allow the
				// event loop to quit naturally
				cpp_conn.close();
				js_conn.close();
			}
		}

		// Start the recursion though all the tests
		runTest();
	});
});

// Invoked by generated code to add test and expected result
// Really constructs list of tests to be sequentially evaluated
function test(testSrc, resSrc) {
	tests.push([testSrc, resSrc])
}

