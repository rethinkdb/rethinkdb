"use strict";

module.exports = parse;

var re_ws = /^\s/,
    re_name = /^(?:\\.|[\w\-\u00c0-\uFFFF])+/,
    re_cleanSelector = /([^\\])\s*([>~+,]|$)\s*/g,
    re_combinators = /^\s*[^\\]\s*[>~+,]|$\s*/g,
    re_escape = /\\([\da-f]{1,6}\s?|(\s)|.)/ig,
    re_comma = /^\s*,\s*/,
    //modified version of https://github.com/jquery/sizzle/blob/master/src/sizzle.js#L87
    re_attr = /^\s*((?:\\.|[\w\u00c0-\uFFFF\-])+)\s*(?:(\S?)=\s*(?:(['"])(.*?)\3|(#?(?:\\.|[\w\u00c0-\uFFFF\-])*)|)|)\s*(i)?\]/;

var actionTypes = {
	__proto__: null,
	"undefined": "exists",
	"":  "equals",
	"~": "element",
	"^": "start",
	"$": "end",
	"*": "any",
	"!": "not",
	"|": "hyphen"
};

var simpleSelectors = {
	__proto__: null,
	">": "child",
	"<": "parent",
	"~": "sibling",
	"+": "adjacent",
	"*": "universal"
};

var attribSelectors = {
	__proto__: null,
	"#": ["id", "equals"],
	".": ["class", "element"]
};

//unescape function taken from https://github.com/jquery/sizzle/blob/master/src/sizzle.js#L139
function funescape( _, escaped, escapedWhitespace ) {
	var high = "0x" + escaped - 0x10000;
	// NaN means non-codepoint
	// Support: Firefox
	// Workaround erroneous numeric interpretation of +"0x"
	return high !== high || escapedWhitespace ?
		escaped :
		// BMP codepoint
		high < 0 ?
			String.fromCharCode( high + 0x10000 ) :
			// Supplemental Plane codepoint (surrogate pair)
			String.fromCharCode( high >> 10 | 0xD800, high & 0x3FF | 0xDC00 );
}

function unescapeCSS(str){
	return str.replace(re_escape, funescape);
}

function getClosingPos(selector){
	var pos = 1, counter = 1, len = selector.length;

	for(; counter > 0 && pos < len; pos++){
		if(selector.charAt(pos) === "(") counter++;
		else if(selector.charAt(pos) === ")") counter--;
	}

	return pos;
}

function parse(selector, options){
	selector = (selector + "").trimLeft().replace(re_cleanSelector, "$1$2");

	var subselects = [],
	    tokens = [],
	    data, firstChar, name;
	
	function getName(){
		var sub = selector.match(re_name)[0];
		selector = selector.substr(sub.length);
		return unescapeCSS(sub);
	}

	function getLCName(){
		var name = getName();

		if(!options || !options.xmlMode){
			name = name.toLowerCase();
		}

		return name;
	}

	while(selector !== ""){
		if(re_name.test(selector)){
			tokens.push({type: "tag", name: getLCName()});
		} else if(re_ws.test(selector)){
			tokens.push({type: "descendant"});
			selector = selector.trimLeft();
		} else {
			firstChar = selector.charAt(0);
			selector = selector.substr(1);

			if(firstChar in simpleSelectors){
				tokens.push({type: simpleSelectors[firstChar]});
			} else if(firstChar in attribSelectors){
				tokens.push({
					type: "attribute",
					name: attribSelectors[firstChar][0],
					action: attribSelectors[firstChar][1],
					value: getName(),
					ignoreCase: false
				});
			} else if(firstChar === "["){
				data = selector.match(re_attr);
				if(!data){
					throw new SyntaxError("Malformed attribute selector: " + selector);
				}
				selector = selector.substr(data[0].length);
				name = unescapeCSS(data[1]);

				if(!options || !options.xmlMode){
					name = name.toLowerCase();
				}

				tokens.push({
					type: "attribute",
					name: name,
					action: actionTypes[data[2]],
					value: unescapeCSS(data[4] || data[5] || ""),
					ignoreCase: !!data[6]
				});
				
			} else if(firstChar === ":"){
				//if(selector.charAt(0) === ":"){} //TODO pseudo-element
				name = getLCName();
				data = null;
				
				if(selector.charAt(0) === "("){
					var pos = getClosingPos(selector);
					data = selector.substr(1, pos - 2);
					selector = selector.substr(pos);
				}
				
				tokens.push({type: "pseudo", name: name, data: data});
			} else if(firstChar === ","){
				if(tokens.length === 0){
					throw new SyntaxError("empty sub-selector");
				}
				subselects.push(tokens);
				tokens = [];
			} else {
				//otherwise, the parser needs to throw or it would enter an infinite loop
				throw new SyntaxError("Unmatched selector: " + firstChar + selector);
			}
		}
	}
	
	if(subselects.length > 0 && tokens.length === 0){
		throw new SyntaxError("empty sub-selector");
	}
	subselects.push(tokens);
	return subselects;
}