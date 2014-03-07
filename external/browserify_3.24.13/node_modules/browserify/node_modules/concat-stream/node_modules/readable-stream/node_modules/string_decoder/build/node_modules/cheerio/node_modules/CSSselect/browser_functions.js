function isTag(elem){
	return elem.nodeType === 1;
}
function getChildren(elem){
	return Array.prototype.slice.call(elem.childNodes, 0);
}
function getParent(elem){
	return elem.parentElement;
}

module.exports = {
	isTag: isTag,
	getSiblings: function(elem){
		var parent = getParent(elem);
		return parent && getChildren(parent);
	},
	getChildren: getChildren,
	getParent: getParent,
	getAttributeValue: function(elem, name){
		return elem.attributes[name].value;
	},
	hasAttrib: function(elem, name){
		return name in elem.attributes;
	},
	getName: function(elem){
		return elem.tagName.toLowerCase();
	},
	findOne: function findOne(test, arr){
		var elem = null;

		for(var i = 0, l = arr.length; i < l && !elem; i++){
			if(test(arr[i])){
				elem = arr[i];
			} else {
				var childs = getChildren(arr[i]);
				if(childs && childs.length > 0){
					elem = findOne(test, childs);
				}
			}
		}

		return elem;
	},
	findAll: function findAll(test, elems){
		var result = [];
		for(var i = 0, j = elems.length; i < j; i++){
			if(!isTag(elems[i])) continue;
			if(test(elems[i])) result.push(elems[i]);
			var childs = getChildren(elems[i]);
			if(childs) result = result.concat(findAll(test, childs));
		}
		return result;
	},
	//https://github.com/ded/qwery/blob/master/pseudos/qwery-pseudos.js#L47-54
	getText: function getText(elem) {
		var str = "",
		    childs = getChildren(elem);

		if(!childs) return str;

		for(var i = 0; i < childs.length; i++){
			if(isTag(childs[i])) str += elem.textContent || elem.innerText || getText(childs[i]);
		}

		return str;
	}
};
