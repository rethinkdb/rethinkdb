"use strict";

var expect = require("expect.js"),
    DomUtils = require("htmlparser2").DomUtils,
	helper = require("../tools/helper.js"),
	path = require("path"),
	document = helper.getDocument(path.join(__dirname, "index.html")),
	CSSselect = helper.CSSselect;

var location = {hash: ""};
CSSselect.pseudos.target = function(elem){
	return elem.attribs && elem.attribs.id === location.hash.substr(1);
};

//---

/*
	The following is taken from https://github.com/ded/qwery/blob/master/tests/tests.js
*/

CSSselect.pseudos.humanoid = function(e) { return CSSselect.is(e, 'li:contains(human)') || CSSselect.is(e, 'ol:contains(human)'); };

var frag = helper.getDOM(
	'<div class="d i v">' +
		'<p id="oooo"><em></em><em id="emem"></em></p>' +
	'</div>' +
	'<p id="sep">' +
		'<div class="a"><span></span></div>' +
	'</p>'
);

var doc = helper.getDOM(
	'<div id="hsoob">' +
		'<div class="a b">' +
			'<div class="d e sib" test="fg" id="booshTest"><p><span id="spanny"></span></p></div>' +
			'<em nopass="copyrighters" rel="copyright booshrs" test="f g" class="sib"></em>' +
			'<span class="h i a sib"></span>' +
		'</div>' +
		'<p class="odd"></p>' +
	'</div>' +
	'<div id="lonelyHsoob"></div>'
);

var el = DomUtils.getElementById('attr-child-boosh', document);

var pseudos = DomUtils.getElementById('pseudos', document).children.filter(DomUtils.isTag);

module.exports = {

'Contexts': {

	'should be able to pass optional context': function () {
		expect(CSSselect('.a', document)).to.have.length(3); //no context found 3 elements (.a)
		expect(CSSselect('.a', CSSselect('#boosh', document))).to.have.length(2); //context found 2 elements (#boosh .a)
	},

/*
	'should be able to pass string as context': function() {
		expect(CSSselect('.a', '#boosh')).to.have.length(2); //context found 2 elements(.a, #boosh)
		expect(CSSselect('.a', '.a')).to.be.empty(); //context found 0 elements(.a, .a)
		expect(CSSselect('.a', '.b')).to.have.length(1); //context found 1 elements(.a, .b)
		expect(CSSselect('.a', '#boosh .b')).to.have.length(1); //context found 1 elements(.a, #boosh .b)
		expect(CSSselect('.b', '#boosh .b')).to.be.empty(); //context found 0 elements(.b, #boosh .b)
	},
*/
/*
	'should be able to pass qwery result as context': function() {
		expect(CSSselect('.a', CSSselect('#boosh', document))).to.have.length(2); //context found 2 elements(.a, #boosh)
		expect(CSSselect('.a', CSSselect('.a', document))).to.be.empty(); //context found 0 elements(.a, .a)
		expect(CSSselect('.a', CSSselect('.b', document))).to.have.length(1); //context found 1 elements(.a, .b)
		expect(CSSselect('.a', CSSselect('#boosh .b', document))).to.have.length(1); //context found 1 elements(.a, #boosh .b)
		expect(CSSselect('.b', CSSselect('#boosh .b', document))).to.be.empty(); //context found 0 elements(.b, #boosh .b)
	},
*/
	'should not return duplicates from combinators': function () {
		expect(CSSselect('#boosh,#boosh', document)).to.have.length(1); //two booshes dont make a thing go right
		expect(CSSselect('#boosh,.apples,#boosh', document)).to.have.length(1); //two booshes and an apple dont make a thing go right
	},

	'byId sub-queries within context': function() {
		expect(CSSselect('#booshTest', CSSselect('#boosh', document))).to.have.length(1); //found "#id #id"
		expect(CSSselect('.a.b #booshTest', CSSselect('#boosh', document))).to.have.length(1); //found ".class.class #id"
		expect(CSSselect('.a>#booshTest', CSSselect('#boosh', document))).to.have.length(1); //found ".class>#id"
		expect(CSSselect('>.a>#booshTest', CSSselect('#boosh', document))).to.have.length(1); //found ">.class>#id"
		expect(CSSselect('#boosh', CSSselect('#booshTest', document)).length).to.not.be.ok(); //shouldn't find #boosh (ancestor) within #booshTest (descendent)
		expect(CSSselect('#boosh', CSSselect('#lonelyBoosh', document)).length).to.not.be.ok(); //shouldn't find #boosh within #lonelyBoosh (unrelated)
	}
},

'CSS 1': {
	'get element by id': function () {
		var result = CSSselect('#boosh', document);
		expect(result[0]).to.be.ok(); //found element with id=boosh
		expect(CSSselect('h1', document)[0]).to.be.ok(); //found 1 h1
	},

	'byId sub-queries': function() {
		expect(CSSselect('#boosh #booshTest', document)).to.have.length(1); //found "#id #id"
		expect(CSSselect('.a.b #booshTest', document)).to.have.length(1); //found ".class.class #id"
		expect(CSSselect('#boosh>.a>#booshTest', document)).to.have.length(1); //found "#id>.class>#id"
		expect(CSSselect('.a>#booshTest', document)).to.have.length(1); //found ".class>#id"
	},

	'get elements by class': function () {
		expect(CSSselect('#boosh .a', document)).to.have.length(2); //found two elements
		expect(CSSselect('#boosh div.a', document)[0]).to.be.ok(); //found one element
		expect(CSSselect('#boosh div', document)).to.have.length(2); //found two {div} elements
		expect(CSSselect('#boosh span', document)[0]).to.be.ok(); //found one {span} element
		expect(CSSselect('#boosh div div', document)[0]).to.be.ok(); //found a single div
		expect(CSSselect('a.odd', document)).to.have.length(1); //found single a
	},

	'combos': function () {
		expect(CSSselect('#boosh div,#boosh span', document)).to.have.length(3); //found 2 divs and 1 span
	},

	'class with dashes': function() {
		expect(CSSselect('.class-with-dashes', document)).to.have.length(1); //found something
	},

	'should ignore comment nodes': function() {
		expect(CSSselect('#boosh *', document)).to.have.length(4); //found only 4 elements under #boosh
	},

	'deep messy relationships': function() {
		// these are mostly characterised by a combination of tight relationships and loose relationships
		// on the right side of the query it's easy to find matches but they tighten up quickly as you
		// go to the left
		// they are useful for making sure the dom crawler doesn't stop short or over-extend as it works
		// up the tree the crawl needs to be comprehensive
		expect(CSSselect('div#fixtures > div a', document)).to.have.length(5); //found four results for "div#fixtures > div a"
		expect(CSSselect('.direct-descend > .direct-descend .lvl2', document)).to.have.length(1); //found one result for ".direct-descend > .direct-descend .lvl2"
		expect(CSSselect('.direct-descend > .direct-descend div', document)).to.have.length(1); //found one result for ".direct-descend > .direct-descend div"
		expect(CSSselect('.direct-descend > .direct-descend div', document)).to.have.length(1); //found one result for ".direct-descend > .direct-descend div"
		expect(CSSselect('div#fixtures div ~ a div', document)).to.be.empty(); //found no results for odd query
		expect(CSSselect('.direct-descend > .direct-descend > .direct-descend ~ .lvl2', document)).to.be.empty(); //found no results for another odd query
	}
},

'CSS 2': {

	'get elements by attribute': function () {
		var wanted = CSSselect('#boosh div[test]', document)[0];
		var expected = DomUtils.getElementById('booshTest', document);
		expect(wanted).to.be(expected); //found attribute
		expect(CSSselect('#boosh div[test=fg]', document)[0]).to.be(expected); //found attribute with value
		expect(CSSselect('em[rel~="copyright"]', document)).to.have.length(1); //found em[rel~="copyright"]
		expect(CSSselect('em[nopass~="copyright"]', document)).to.be.empty(); //found em[nopass~="copyright"]
	},

	'should not throw error by attribute selector': function () {
		expect(CSSselect('[foo^="bar"]', document)).to.have.length(1); //found 1 element
	},

	'crazy town': function () {
		var el = DomUtils.getElementById('attr-test3', document);
		expect(CSSselect('div#attr-test3.found.you[title="whatup duders"]', document)[0]).to.be(el); //found the right element
	}

},

'attribute selectors': {

	/* CSS 2 SPEC */

	'[attr]': function () {
		var expected = DomUtils.getElementById('attr-test-1', document);
		expect(CSSselect('#attributes div[unique-test]', document)[0]).to.be(expected); //found attribute with [attr]
	},

	'[attr=val]': function () {
		var expected = DomUtils.getElementById('attr-test-2', document);
		expect(CSSselect('#attributes div[test="two-foo"]', document)[0]).to.be(expected); //found attribute with =
		expect(CSSselect("#attributes div[test='two-foo']", document)[0]).to.be(expected); //found attribute with =
		expect(CSSselect('#attributes div[test=two-foo]', document)[0]).to.be(expected); //found attribute with =
	},

	'[attr~=val]': function () {
		var expected = DomUtils.getElementById('attr-test-3', document);
		expect(CSSselect('#attributes div[test~=three]', document)[0]).to.be(expected); //found attribute with ~=
	},

	'[attr|=val]': function () {
		var expected = DomUtils.getElementById('attr-test-2', document);
		expect(CSSselect('#attributes div[test|="two-foo"]', document)[0]).to.be(expected); //found attribute with |=
		expect(CSSselect('#attributes div[test|=two]', document)[0]).to.be(expected); //found attribute with |=
	},

	'[href=#x] special case': function () {
		var expected = DomUtils.getElementById('attr-test-4', document);
		expect(CSSselect('#attributes a[href="#aname"]', document)[0]).to.be(expected); //found attribute with href=#x
	},

	/* CSS 3 SPEC */

	'[attr^=val]': function () {
		var expected = DomUtils.getElementById('attr-test-2', document);
		expect(CSSselect('#attributes div[test^=two]', document)[0]).to.be(expected); //found attribute with ^=
	},

	'[attr$=val]': function () {
		var expected = DomUtils.getElementById('attr-test-2', document);
		expect(CSSselect('#attributes div[test$=foo]', document)[0]).to.be(expected); //found attribute with $=
	},

	'[attr*=val]': function () {
		var expected = DomUtils.getElementById('attr-test-3', document);
		expect(CSSselect('#attributes div[test*=hree]', document)[0]).to.be(expected); //found attribute with *=
	},

	'direct descendants': function () {
		expect(CSSselect('#direct-descend > .direct-descend', document)).to.have.length(2); //found two direct descendents
		expect(CSSselect('#direct-descend > .direct-descend > .lvl2', document)).to.have.length(3); //found three second-level direct descendents
	},

	'sibling elements': function () {
		expect(CSSselect('#sibling-selector ~ .sibling-selector', document)).to.have.length(2); //found two siblings
		expect(CSSselect('#sibling-selector ~ div.sibling-selector', document)).to.have.length(2); //found two siblings
		expect(CSSselect('#sibling-selector + div.sibling-selector', document)).to.have.length(1); //found one sibling
		expect(CSSselect('#sibling-selector + .sibling-selector', document)).to.have.length(1); //found one sibling

		expect(CSSselect('.parent .oldest ~ .sibling', document)).to.have.length(4); //found four younger siblings
		expect(CSSselect('.parent .middle ~ .sibling', document)).to.have.length(2); //found two younger siblings
		expect(CSSselect('.parent .middle ~ h4', document)).to.have.length(1); //found next sibling by tag
		expect(CSSselect('.parent .middle ~ h4.younger', document)).to.have.length(1); //found next sibling by tag and class
		expect(CSSselect('.parent .middle ~ h3', document)).to.be.empty(); //an element can't be its own sibling
		expect(CSSselect('.parent .middle ~ h2', document)).to.be.empty(); //didn't find an older sibling
		expect(CSSselect('.parent .youngest ~ .sibling', document)).to.be.empty(); //found no younger siblings

		expect(CSSselect('.parent .oldest + .sibling', document)).to.have.length(1); //found next sibling
		expect(CSSselect('.parent .middle + .sibling', document)).to.have.length(1); //found next sibling
		expect(CSSselect('.parent .middle + h4', document)).to.have.length(1); //found next sibling by tag
		expect(CSSselect('.parent .middle + h3', document)).to.be.empty(); //an element can't be its own sibling
		expect(CSSselect('.parent .middle + h2', document)).to.be.empty(); //didn't find an older sibling
		expect(CSSselect('.parent .youngest + .sibling', document)).to.be.empty(); //found no younger siblings
	}

},

/*
'Uniq': {
	'duplicates arent found in arrays': function () {
		expect(CSSselect.uniq(['a', 'b', 'c', 'd', 'e', 'a', 'b', 'c', 'd', 'e'])).to.have.length(5); //result should be a, b, c, d, e
		expect(CSSselect.uniq(['a', 'b', 'c', 'c', 'c'])).to.have.length(3); //result should be a, b, c
	}
},
*/


'element-context queries': {

/*
	'relationship-first queries': function() {
		expect(CSSselect('> .direct-descend', CSSselect('#direct-descend', document))).to.have.length(2); //found two direct descendents using > first
		expect(CSSselect('~ .sibling-selector', CSSselect('#sibling-selector', document))).to.have.length(2); //found two siblings with ~ first
		expect(CSSselect('+ .sibling-selector', CSSselect('#sibling-selector', document))).to.have.length(1); //found one sibling with + first
		expect(CSSselect('> .tokens a', CSSselect('.idless', document)[0])).to.have.length(1); //found one sibling from a root with no id
	},
*/

	// should be able to query on an element that hasn't been inserted into the dom
	'detached fragments': function() {
		expect(CSSselect('.a span', frag)).to.have.length(1); //should find child elements of fragment
		//expect(CSSselect('> div p em', frag)).to.have.length(2); //should find child elements of fragment, relationship first
	},

	'byId sub-queries within detached fragment': function () {
		expect(CSSselect('#emem', frag)).to.have.length(1); //found "#id" in fragment
		expect(CSSselect('.d.i #emem', frag)).to.have.length(1); //found ".class.class #id" in fragment
		expect(CSSselect('.d #oooo #emem', frag)).to.have.length(1); //found ".class #id #id" in fragment
		//expect(CSSselect('> div #oooo', frag)).to.have.length(1); //found "> .class #id" in fragment
		expect(CSSselect('#oooo', CSSselect('#emem', frag)).length).to.not.be.ok(); //shouldn't find #oooo (ancestor) within #emem (descendent)
		expect(CSSselect('#sep', CSSselect('#emem', frag)).length).to.not.be.ok(); //shouldn't find #sep within #emem (unrelated)
	},

/*
	'exclude self in match': function() {
		expect(CSSselect('.order-matters', CSSselect('#order-matters', document))).to.have.length(4); //should not include self in element-context queries
	},
*/

	// because form's have .length
	'forms can be used as contexts': function() {
		expect(CSSselect('*', CSSselect('form', document)[0])).to.have.length(3); //found 3 elements under &lt;form&gt;
	}
},

'tokenizer': {

	'should not get weird tokens': function () {
		expect(CSSselect('div .tokens[title="one"]', document)[0]).to.be(DomUtils.getElementById('token-one', document)); //found div .tokens[title="one"]
		expect(CSSselect('div .tokens[title="one two"]', document)[0]).to.be(DomUtils.getElementById('token-two', document)); //found div .tokens[title="one two"]
		expect(CSSselect('div .tokens[title="one two three #%"]', document)[0]).to.be(DomUtils.getElementById('token-three', document)); //found div .tokens[title="one two three #%"]
		expect(CSSselect("div .tokens[title='one two three #%'] a", document)[0]).to.be(DomUtils.getElementById('token-four', document)); //found div .tokens[title=\'one two three #%\'] a
		expect(CSSselect('div .tokens[title="one two three #%"] a[href$=foo] div', document)[0]).to.be(DomUtils.getElementById('token-five', document)); //found div .tokens[title="one two three #%"] a[href=foo] div
	}

},

'interesting syntaxes': {
	'should parse bad selectors': function () {
		expect(CSSselect('#spaced-tokens    p    em    a', document).length).to.be.ok(); //found element with funny tokens
	}
},

'order matters': {

	// <div id="order-matters">
	//   <p class="order-matters"></p>
	//   <a class="order-matters">
	//     <em class="order-matters"></em><b class="order-matters"></b>
	//   </a>
	// </div>

	'the order of elements return matters': function () {
		function tag(el) {
			return el.name.toLowerCase();
		}
		var els = CSSselect('#order-matters .order-matters', document);
		expect(tag(els[0])).to.be('p'); //first element matched is a {p} tag
		expect(tag(els[1])).to.be('a'); //first element matched is a {a} tag
		expect(tag(els[2])).to.be('em'); //first element matched is a {em} tag
		expect(tag(els[3])).to.be('b'); //first element matched is a {b} tag
	}

},

'pseudo-selectors': {
	':contains': function() {
		expect(CSSselect('li:contains(humans)', document)).to.have.length(1); //found by "element:contains(text)"
		expect(CSSselect(':contains(humans)', document)).to.have.length(5); //found by ":contains(text)", including all ancestors
		// * is an important case, can cause weird errors
		expect(CSSselect('*:contains(humans)', document)).to.have.length(5); //found by "*:contains(text)", including all ancestors
		expect(CSSselect('ol:contains(humans)', document)).to.have.length(1); //found by "ancestor:contains(text)"
	},

	':not': function() {
		expect(CSSselect('.odd:not(div)', document)).to.have.length(1); //found one .odd :not an &lt;a&gt;
	},

	':first-child': function () {
		expect(CSSselect('#pseudos div:first-child', document)[0]).to.be(pseudos[0]); //found first child
		expect(CSSselect('#pseudos div:first-child', document)).to.have.length(1); //found only 1
	},

	':last-child': function () {
		var all = DomUtils.getElementsByTagName('div', pseudos);
		expect(CSSselect('#pseudos div:last-child', document)[0]).to.be(all[all.length - 1]); //found last child
		expect(CSSselect('#pseudos div:last-child', document)).to.have.length(1); //found only 1
	},

	'ol > li[attr="boosh"]:last-child': function () {
		var expected = DomUtils.getElementById('attr-child-boosh', document);
		expect(CSSselect('ol > li[attr="boosh"]:last-child', document)).to.have.length(1); //only 1 element found
		expect(CSSselect('ol > li[attr="boosh"]:last-child', document)[0]).to.be(expected); //found correct element
	},

	':nth-child(odd|even|x)': function () {
		var second = DomUtils.getElementsByTagName('div', pseudos)[1];
		expect(CSSselect('#pseudos :nth-child(odd)', document)).to.have.length(4); //found 4 odd elements
		expect(CSSselect('#pseudos div:nth-child(odd)', document)).to.have.length(3); //found 3 odd elements with div tag
		expect(CSSselect('#pseudos div:nth-child(even)', document)).to.have.length(3); //found 3 even elements with div tag
		expect(CSSselect('#pseudos div:nth-child(2)', document)[0]).to.be(second); //found 2nd nth-child of pseudos
	},

	':nth-child(expr)': function () {
		var fifth = DomUtils.getElementsByTagName('a', pseudos)[0];
		var sixth = DomUtils.getElementsByTagName('div', pseudos)[4];

		expect(CSSselect('#pseudos :nth-child(3n+1)', document)).to.have.length(3); //found 3 elements
		expect(CSSselect('#pseudos :nth-child(+3n-2)', document)).to.have.length(3); //found 3 elements'
		expect(CSSselect('#pseudos :nth-child(-n+6)', document)).to.have.length(6); //found 6 elements
		expect(CSSselect('#pseudos :nth-child(-n+5)', document)).to.have.length(5); //found 5 elements
		expect(CSSselect('#pseudos :nth-child(3n+2)', document)[1]).to.be(fifth); //second :nth-child(3n+2) is the fifth child
		expect(CSSselect('#pseudos :nth-child(3n)', document)[1]).to.be(sixth); //second :nth-child(3n) is the sixth child
	},

	':nth-last-child(odd|even|x)': function () {
		var second = DomUtils.getElementsByTagName('div', pseudos)[1];
		expect(CSSselect('#pseudos :nth-last-child(odd)', document)).to.have.length(4); //found 4 odd elements
		expect(CSSselect('#pseudos div:nth-last-child(odd)', document)).to.have.length(3); //found 3 odd elements with div tag
		expect(CSSselect('#pseudos div:nth-last-child(even)', document)).to.have.length(3); //found 3 even elements with div tag
		expect(CSSselect('#pseudos div:nth-last-child(6)', document)[0]).to.be(second); //6th nth-last-child should be 2nd of 7 elements
	},

	':nth-last-child(expr)': function () {
		var third = DomUtils.getElementsByTagName('div', pseudos)[2];

		expect(CSSselect('#pseudos :nth-last-child(3n+1)', document)).to.have.length(3); //found 3 elements
		expect(CSSselect('#pseudos :nth-last-child(3n-2)', document)).to.have.length(3); //found 3 elements
		expect(CSSselect('#pseudos :nth-last-child(-n+6)', document)).to.have.length(6); //found 6 elements
		expect(CSSselect('#pseudos :nth-last-child(-n+5)', document)).to.have.length(5); //found 5 elements
		expect(CSSselect('#pseudos :nth-last-child(3n+2)', document)[0]).to.be(third); //first :nth-last-child(3n+2) is the third child
	},

	':nth-of-type(expr)': function () {
		var a = DomUtils.getElementsByTagName('a', pseudos)[0];

		expect(CSSselect('#pseudos div:nth-of-type(3n+1)', document)).to.have.length(2); //found 2 div elements
		expect(CSSselect('#pseudos a:nth-of-type(3n+1)', document)).to.have.length(1); //found 1 a element
		expect(CSSselect('#pseudos a:nth-of-type(3n+1)', document)[0]).to.be(a); //found the right a element
		expect(CSSselect('#pseudos a:nth-of-type(3n)', document)).to.be.empty(); //no matches for every third a
		expect(CSSselect('#pseudos a:nth-of-type(odd)', document)).to.have.length(1); //found the odd a
		expect(CSSselect('#pseudos a:nth-of-type(1)', document)).to.have.length(1); //found the first a
	},

	':nth-last-of-type(expr)': function () {
		var second = DomUtils.getElementsByTagName('div', pseudos)[1];

		expect(CSSselect('#pseudos div:nth-last-of-type(3n+1)', document)).to.have.length(2); //found 2 div elements
		expect(CSSselect('#pseudos a:nth-last-of-type(3n+1)', document)).to.have.length(1); //found 1 a element
		expect(CSSselect('#pseudos div:nth-last-of-type(5)', document)[0]).to.be(second); //5th nth-last-of-type should be 2nd of 7 elements
	},

	':first-of-type': function () {
		expect(CSSselect('#pseudos a:first-of-type', document)[0]).to.be(DomUtils.getElementsByTagName('a', pseudos)[0]); //found first a element
		expect(CSSselect('#pseudos a:first-of-type', document)).to.have.length(1); //found only 1
	},

	':last-of-type': function () {
		var all = DomUtils.getElementsByTagName('div', pseudos);
		expect(CSSselect('#pseudos div:last-of-type', document)[0]).to.be(all[all.length - 1]); //found last div element
		expect(CSSselect('#pseudos div:last-of-type', document)).to.have.length(1); //found only 1
	},

	':only-of-type': function () {
		expect(CSSselect('#pseudos a:only-of-type', document)[0]).to.be(DomUtils.getElementsByTagName('a', pseudos)[0]); //found the only a element
		expect(CSSselect('#pseudos a:first-of-type', document)).to.have.length(1); //found only 1
	},

	':target': function () {
		location.hash = '';
		expect(CSSselect('#pseudos:target', document)).to.be.empty(); //#pseudos is not the target
		location.hash = '#pseudos';
		expect(CSSselect('#pseudos:target', document)).to.have.length(1); //now #pseudos is the target
		location.hash = '';
	},

	'custom pseudos': function() {
		// :humanoid implemented just for testing purposes
		expect(CSSselect(':humanoid', document)).to.have.length(2); //selected using custom pseudo
	}

},

/*
'argument types': {

	'should be able to pass in nodes as arguments': function () {
		var el = DomUtils.getElementById('boosh', document);
		expect(CSSselect(el)[0]).to.be(el); //CSSselect(el)[0] == el
		expect(CSSselect(el, 'body')[0]).to.be(el); //CSSselect(el, 'body')[0] == el
		expect(CSSselect(el, document)[0]).to.be(el); //CSSselect(el, document)[0] == el
		expect(CSSselect(window)[0]).to.be(window); //CSSselect(window)[0] == window
		expect(CSSselect(document)[0]).to.be(document); //CSSselect(document)[0] == document
	},

	'should be able to pass in an array of results as arguments': function () {
		var el = DomUtils.getElementById('boosh', document);
		var result = CSSselect([CSSselect('#boosh', document), CSSselect(document), CSSselect(window)]);
		expect(result).to.have.length(3); //3 elements in the combined set
		expect(result[0]).to.be(el); //result[0] == el
		expect(result[1]).to.be(document); //result[0] == document
		expect(result[2]).to.be(window); //result[0] == window
		expect(CSSselect([CSSselect('#pseudos div.odd', document), CSSselect('#pseudos div.even', document)])).to.have.length(6); //found all the odd and even divs
	}

},
*/

'is()': {
	'simple selectors': function () {
		expect(CSSselect.is(el, 'li')).to.be.ok(); //tag
		expect(CSSselect.is(el, '*')).to.be.ok(); //wildcard
		expect(CSSselect.is(el, '#attr-child-boosh')).to.be.ok(); //#id
		expect(CSSselect.is(el, '[attr]')).to.be.ok(); //[attr]
		expect(CSSselect.is(el, '[attr=boosh]')).to.be.ok(); //[attr=val]
		expect(CSSselect.is(el, 'div')).to.not.be.ok(); //wrong tag
		expect(CSSselect.is(el, '#foo')).to.not.be.ok(); //wrong #id
		expect(CSSselect.is(el, '[foo]')).to.not.be.ok(); //wrong [attr]
		expect(CSSselect.is(el, '[attr=foo]')).to.not.be.ok(); //wrong [attr=val]
	},
	'selector sequences': function () {
		expect(CSSselect.is(el, 'li#attr-child-boosh[attr=boosh]')).to.be.ok(); //tag#id[attr=val]
		expect(CSSselect.is(el, 'div#attr-child-boosh[attr=boosh]')).to.not.be.ok(); //wrong tag#id[attr=val]
	},
	'selector sequences combinators': function () {
		expect(CSSselect.is(el, 'ol li')).to.be.ok(); //tag tag
		expect(CSSselect.is(el, 'ol>li')).to.be.ok(); //tag>tag
		expect(CSSselect.is(el, 'ol>li+li')).to.be.ok(); //tab>tag+tag
		expect(CSSselect.is(el, 'ol#list li#attr-child-boosh[attr=boosh]')).to.be.ok(); //tag#id tag#id[attr=val]
		expect(CSSselect.is(el, 'ol#list>li#attr-child-boosh[attr=boosh]')).to.not.be.ok(); //wrong tag#id>tag#id[attr=val]
		expect(CSSselect.is(el, 'ol ol li#attr-child-boosh[attr=boosh]')).to.be.ok(); //tag tag tag#id[attr=val]
		expect(CSSselect.is(CSSselect('#token-four', document)[0], 'div#fixtures>div a')).to.be.ok(); //tag#id>tag tag where ambiguous middle tag requires backtracking
	},
	'pseudos': function() {
		//TODO: more tests!
		expect(CSSselect.is(el, 'li:contains(hello)')).to.be.ok(); //matching :contains(text)
		expect(CSSselect.is(el, 'li:contains(human)')).to.not.be.ok(); //non-matching :contains(text)
		expect(CSSselect.is(CSSselect('#list>li', document)[2], ':humanoid')).to.be.ok(); //matching custom pseudo
		expect(CSSselect.is(CSSselect('#list>li', document)[1], ':humanoid')).to.not.be.ok(); //non-matching custom pseudo
	}/*,
	'context': function () {
		expect(CSSselect.is(el, 'li#attr-child-boosh[attr=boosh]', CSSselect('#list', document)[0])).to.be.ok(); //context
		expect(CSSselect.is(el, 'ol#list li#attr-child-boosh[attr=boosh]', CSSselect('#boosh', document)[0])).to.not.be.ok(); //wrong context
	}*/
},

'selecting elements in other documents': {
	'get element by id': function () {
		var result = CSSselect('#hsoob', doc);
		expect(result[0]).to.be.ok(); //found element with id=hsoob
	},

	'get elements by class': function () {
		expect(CSSselect('#hsoob .a', doc)).to.have.length(2); //found two elements
		expect(CSSselect('#hsoob div.a', doc)[0]).to.be.ok(); //found one element
		expect(CSSselect('#hsoob div', doc)).to.have.length(2); //found two {div} elements
		expect(CSSselect('#hsoob span', doc)[0]).to.be.ok(); //found one {span} element
		expect(CSSselect('#hsoob div div', doc)[0]).to.be.ok(); //found a single div
		expect(CSSselect('p.odd', doc)).to.have.length(1); //found single br
	},

	'complex selectors': function () {
		expect(CSSselect('.d ~ .sib', doc)).to.have.length(2); //found one ~ sibling
		expect(CSSselect('.a .d + .sib', doc)).to.have.length(1); //found 2 + siblings
		expect(CSSselect('#hsoob > div > .h', doc)).to.have.length(1); //found span using child selectors
		expect(CSSselect('.a .d ~ .sib[test="f g"]', doc)).to.have.length(1); //found 1 ~ sibling with test attribute
	},

	'byId sub-queries': function () {
		expect(CSSselect('#hsoob #spanny', doc)).to.have.length(1); //found "#id #id" in frame
		expect(CSSselect('.a #spanny', doc)).to.have.length(1); //found ".class #id" in frame
		expect(CSSselect('.a #booshTest #spanny', doc)).to.have.length(1); //found ".class #id #id" in frame
		//expect(CSSselect('> #hsoob', doc)).to.have.length(1) //found "> #id" in frame
	},

	'byId sub-queries within sub-context': function () {
		expect(CSSselect('#spanny', CSSselect('#hsoob', doc))).to.have.length(1); //found "#id -> #id" in frame
		expect(CSSselect('.a #spanny', CSSselect('#hsoob', doc))).to.have.length(1); //found ".class #id" in frame
		expect(CSSselect('.a #booshTest #spanny', CSSselect('#hsoob', doc))).to.have.length(1); //found ".class #id #id" in frame
		expect(CSSselect('.a > #booshTest', CSSselect('#hsoob', doc))).to.have.length(1); //found "> .class #id" in frame
		expect(CSSselect('#booshTest', CSSselect('#spanny', doc)).length).to.not.be.ok(); //shouldn't find #booshTest (ancestor) within #spanny (descendent)
		expect(CSSselect('#booshTest', CSSselect('#lonelyHsoob', doc)).length).to.not.be.ok(); //shouldn't find #booshTest within #lonelyHsoob (unrelated)
	}

}

};
