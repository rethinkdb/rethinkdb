describe("nwmatcher", function(){
	require("./nwmatcher/");
});

describe("sizzle", function(){
	describe("selector", function(){
		require("./sizzle/selector");
	});
});

describe("qwery", function(){
	exportsRun(require("./qwery/"));
});

function exportsRun(mod){
	Object.keys(mod).forEach(function(name){
		if(typeof mod[name] === "object") describe(name, function(){
				exportsRun(mod[name]);
			});
		else it(name, mod[name]);
	});
}