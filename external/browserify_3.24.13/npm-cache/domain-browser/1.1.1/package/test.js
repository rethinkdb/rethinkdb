// Import
var expect = require('chai').expect;
var joe = require('joe');
var domain = require('./');

// =====================================
// Tests

joe.describe('domain-browser', function(describe,it){
	it('should work on throws', function(done){
		var d = domain.create();
		d.on('error', function(err){
			expect(err && err.message).to.eql('a thrown error');
			done();
		});
		d.run(function(){
			throw new Error('a thrown error');
		});
	});
});