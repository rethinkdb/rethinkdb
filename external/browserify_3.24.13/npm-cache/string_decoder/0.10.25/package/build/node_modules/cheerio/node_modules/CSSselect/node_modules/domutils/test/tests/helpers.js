var makeDom = require("../utils").makeDom;
var assert = require("assert");

describe("helpers", function() {
	describe("removeSubsets", function() {
		var removeSubsets = require("../..").removeSubsets;
		var dom = makeDom("<div><p><span></span></p><p></p></div>")[0];

		it("removes identical trees", function() {
			var matches = removeSubsets([dom, dom]);
			assert.equal(matches.length, 1);
		});

		it("Removes subsets found first", function() {
			var matches = removeSubsets([dom, dom.children[0].children[0]]);
			assert.equal(matches.length, 1);
		});

		it("Removes subsets found last", function() {
			var matches = removeSubsets([dom.children[0], dom]);
			assert.equal(matches.length, 1);
		});

		it("Does not remove unique trees", function() {
			var matches = removeSubsets([dom.children[0], dom.children[1]]);
			assert.equal(matches.length, 2);
		});
	});
});
