var CSSselect = require(".."),
    htmlparser = require("htmlparser2"),
    assert = require("assert");

function makeDom(markup) {
	var handler = new htmlparser.DomHandler(),
		parser = new htmlparser.Parser(handler);
	parser.write(markup);
	parser.done();
	return handler.dom;
}

describe("API", function() {
	describe("removes duplicates", function() {
		it("between identical trees", function() {
			var dom = makeDom("<div></div>")[0];
			var matches = CSSselect("div", [dom, dom]);
			assert.equal(matches.length, 1, "Removes duplicate matches");
		});
		it("between a superset and subset", function() {
			var dom = makeDom("<div><p></p></div>")[0];
			var matches = CSSselect("p", [dom, dom.children[0]]);
			assert.equal(matches.length, 1, "Removes duplicate matches");
		});
		it("betweeen a subset and superset", function() {
			var dom = makeDom("<div><p></p></div>")[0];
			var matches = CSSselect("p", [dom.children[0], dom]);
			assert.equal(matches.length, 1, "Removes duplicate matches");
		});
	});
});
