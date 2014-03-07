module.exports = sortByProcedure;

/*
	sort the parts of the passed selector,
	as there is potential for optimization
	(some types of selectors are faster than others)
*/

var procedure = {
	__proto__: null,
	universal: 5, //should be last so that it can be ignored
	tag: 3, //very quick test
	attribute: 1, //can be faster than class
	pseudo: 0, //can be pretty expensive (especially :has)

	//everything else shouldn't be moved
	descendant: -1,
	child: -1,
	sibling: -1,
	adjacent: -1
};

function sortByProcedure(arr){
	//TODO sort individual attribute selectors
	for(var i = 1; i < arr.length; i++){
		var procNew = procedure[arr[i].type];

		if(procNew !== -1){
			for(var j = i - 1; j >= 0 && procNew < procedure[arr[j].type]; j--){
				var tmp = arr[j + 1];
				arr[j + 1] = arr[j];
				arr[j] = tmp;
			}
		}
	}
	return arr;
}