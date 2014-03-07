var helper = require("./helper.js"),
	doc = helper.getFile("W3C_Selectors.html"),
	CSSselect = helper.CSSselect,
	soupselect = require("cheerio-soupselect"),
	selectors = ["body", "div", "body div", "div p", "div > p", "div + p", "div ~ p", "div[class^=exa][class$=mple]", "div p a", "div, p, a", ".note", "div.example", "ul .tocline2", "div.example, div.note", "#title", "h1#title", "div #title", "ul.toc li.tocline2", "ul.toc > li.tocline2", "h1#title + div > p", "h1[id]:contains(Selectors)", "a[href][lang][class]", "div[class]", "div[class=example]", "div[class^=exa]", "div[class$=mple]", "div[class*=e]", "div[class|=dialog]", "div[class!=made_up]", "div[class~=example]"/*, "div:not(.example)", "p:contains(selectors)", "p:nth-child(even)", "p:nth-child(2n)", "p:nth-child(odd)", "p:nth-child(2n+1)", "p:nth-child(n)", "p:only-child", "p:last-child", "p:first-child"*/];

var engines = [function(a,b){return CSSselect(b,a);}, soupselect.select];

//returns true when an error occurs
function testResult(rule, index){
	var results = engines
		.map(function(func){ return func(doc, rule); });

	//check if both had the same result
	for(var i = 1; i < results.length; i++){
		//TODO: might be hard to debug with more engines
		if(results[i-1].length !== results[i].length){
			//console.log(rule, results[i-1].length, results[i].length);
			return true;
		}
		for(var j = 0; j < results[i].length; j++){
			if(results[i-1][j] !== results[i][j]){
				if(results[i-1].indexOf(results[i][j]) === -1){
					return true;
				}
			}
		}
		//require("assert").deepEqual(results[i-1], results[i], rule + ": not the same elements");
	}
	
	return false;
}

selectors.filter(testResult).forEach(function(rule){ print(rule, "failed!\n"); });

process.exit(0); //don't run speed tests

print("-----\n\nChecking performance\n\n");

//test the speed
var ben = require("ben");

function testSpeed(rule){
	print(rule, Array(28-rule.length).join(" "));

	var results = engines
		.map(function(func){ return function(){ return func(doc, rule); }});
	
	//also add a precompiled CSSselect test
	var compiled = CSSselect(rule);
	results.unshift(function(){ return CSSselect.iterate(compiled, doc); });
	
	results = results.map(ben);
	
	var min = Math.min.apply(null, results);
	var max = Math.max.apply(null, results);

	results.forEach(function(result){
		if(result === min) return print(" +", result, "+");
		if(result === max) return print(" !", result, "!");
		if(Math.abs(result-min) > Math.abs(result-max)){
			return print(" =", result, "=");
		}
		print(" ~", result, "~");
	});
	
	print("\n");
}

print("RULE                    ", "CSSselect (pc)", "CSSselect", "soupselect\n");

selectors.forEach(testSpeed);

function print(){
	process.stdout.write(Array.prototype.join.call(arguments, " "));
}