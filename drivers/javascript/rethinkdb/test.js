
goog.provide('rethinkdb_test');

goog.require('goog.testing.jsunit');
goog.require('goog.testing.AsyncTestCase');

var asyncTestCase = goog.testing.AsyncTestCase.createAndInstall();
var conn = rethinkdb.net.connect('newton');
var q = rethinkdb.query;

function aeq(eq) {
	asyncTestCase.waitForAsync('');
	return function(val) {
		assertEquals('test', eq, val);
		asyncTestCase.continueTesting();
	};
}

function testBasic() {
    q(1).run(aeq(1));
}
