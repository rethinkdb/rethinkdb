exports.find = function(test, arr, recurse, limit){
	var result = [], childs;

	for(var i = 0, j = arr.length; i < j; i++){
		if(test(arr[i])){
			result.push(arr[i]);
			if(--limit <= 0) break;
		}

		childs = this.getChildren(arr[i]);
		if(recurse && childs && childs.length > 0){
			childs = this.find(test, childs, recurse, limit);
			result = result.concat(childs);
			limit -= childs.length;
			if(limit <= 0) break;
		}
	}

	return result;
};

exports.findOneChild = function(test, arr){
	for(var i = 0, l = arr.length; i < l; i++){
		if(test(arr[i])) return arr[i];
	}

	return null;
};

exports.findOne = function(test, arr){
	var elem = null;

	for(var i = 0, l = arr.length; i < l && !elem; i++){
		if(test(arr[i])){
			elem = arr[i];
		} else if(arr[i].children && arr[i].children.length > 0){
			elem = this.findOne(test, arr[i].children);
		}
	}

	return elem;
};

exports.findAll = function(test, elems){
	var result = [];
	for(var i = 0, j = elems.length; i < j; i++){
		if(test(elems[i])) result.push(elems[i]);

		var childs = this.getChildren(elems[i]);
		if(childs && childs.length){
			result = result.concat(this.findAll(test, childs));
		}
	}
	return result;
};

exports.filter = function(test, element, recurse, limit){
	if(!Array.isArray(element)) element = [element];

	if(typeof limit !== "number" || !isFinite(limit)){
		if(recurse === false){
			return element.filter(test);
		} else {
			return this.findAll(test, element);
		}
	} else if(limit === 1){
		if(recurse === false){
			element = this.findOneChild(test, element);
		} else {
			element = this.findOne(test, element);
		}
		return element ? [element] : [];
	} else {
		return this.find(test, element, recurse !== false, limit);
	}
};
