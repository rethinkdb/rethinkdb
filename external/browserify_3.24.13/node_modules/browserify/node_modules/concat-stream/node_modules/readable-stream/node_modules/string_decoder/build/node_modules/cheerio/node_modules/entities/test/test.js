var assert = require("assert");
var entities = require('../');

describe("Encode->decode test", function() {
	var testcases = [
		{
			input: "asdf & ÿ ü '",
			xml: "asdf &amp; &#255; &#252; &apos;",
			html4: "asdf &amp; &yuml; &uuml; &apos;",
			html5: "asdf &amp; &yuml; &uuml; &apos;"
		}, {
			input: "&#38;",
			xml: "&amp;#38;",
			html4: "&amp;#38;",
			html5: "&amp;&num;38&semi;"
		},
	];
	testcases.forEach(function(tc) {
		var encodedXML = entities.encodeXML(tc.input);
		it("should XML encode " + tc.input, function() {
			assert.equal(encodedXML, tc.xml);
		});
		it("should XML decode " + encodedXML, function() {
			assert.equal(entities.decodeXML(encodedXML), tc.input);
		});
		var encodedHTML4 = entities.encodeHTML4(tc.input);
		it("should HTML4 encode " + tc.input, function() {
			assert.equal(encodedHTML4, tc.html4);
		});
		it("should HTML4 decode " + encodedHTML4, function() {
			assert.equal(entities.decodeHTML4(encodedHTML4), tc.input);
		});
		var encodedHTML5 = entities.encodeHTML5(tc.input);
		it("should HTML5 encode " + tc.input, function() {
			assert.equal(encodedHTML5, tc.html5);
		});
		it("should HTML5 decode " + encodedHTML5, function() {
			assert.equal(entities.decodeHTML5(encodedHTML5), tc.input);
		});
	});
});

describe("Decode test", function() {
	var testcases = [
		{ input: "&amp;amp;",  output: "&amp;" },
		{ input: "&amp;#38;",  output: "&#38;" },
		{ input: "&amp;#x26;", output: "&#x26;" },
		{ input: "&amp;#X26;", output: "&#X26;" },
		{ input: "&#38;#38;",  output: "&#38;" },
		{ input: "&#x26;#38;", output: "&#38;" },
		{ input: "&#X26;#38;", output: "&#38;" },
		{ input: "&#x3a;",     output: ":" },
		{ input: "&#x3A;",     output: ":" },
		{ input: "&#X3a;",     output: ":" },
		{ input: "&#X3A;",     output: ":" }
	];
	testcases.forEach(function(tc) {
		it("should XML decode " + tc.input, function() {
			assert.equal(entities.decodeXML(tc.input), tc.output);
		});
		it("should HTML4 decode " + tc.input, function() {
			assert.equal(entities.decodeHTML4(tc.input), tc.output);
		});
		it("should HTML5 decode " + tc.input, function() {
			assert.equal(entities.decodeHTML5(tc.input), tc.output);
		});
	});
});
