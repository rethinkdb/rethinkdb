var DomUtils = module.exports;

[
	"stringify", "traversal", "manipulation", "querying", "legacy", "helpers"
].forEach(function(name){
	var ext = require("./lib/" + name);
	Object.keys(ext).forEach(function(key){
		DomUtils[key] = ext[key].bind(DomUtils);
	});
});
