var foo = require('./foo.js');

var stringify = JSON.stringify;
var bundleFn = arguments[3];
var args = bundleFn.arguments;
var sources = args[0];

var src = '(' + bundleFn + ')({';
var first = true;
for (var key in sources) {
    src += (first ? '' : ',') + stringify(key) + ':['
        + sources[key][0] + ',' + stringify(sources[key][1])
        + ']'
    ;
    first = false;
}
src += '},{},' + stringify(args[2]) + ')';
console.log(src);
