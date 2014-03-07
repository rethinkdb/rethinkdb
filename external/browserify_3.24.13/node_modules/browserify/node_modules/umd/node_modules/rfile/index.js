var callsite = require('callsite');
var resolve = require('resolve');
var dirname = require('path').dirname;
var read = require('fs').readFileSync;

exports = module.exports = req;
exports.resolve = res;

function req(pkg, options) {
  options = options || {};
  var path = res(pkg, options);
  return options.binary ? read(path) : fixup(read(path));
}

function res(pkg, options) {
  options = options || {};
  options.basedir = options.basedir || directory(options.exclude);
  options.extensions = options.extensions || ['.js', '.json'];
  return resolve.sync(pkg, options);
}

function directory(exclude) {
  var stack = callsite();
  for (var i = 0; i < stack.length; i++) {
    var filename = stack[i].getFileName();
    if (filename !== __filename && (!exclude || (exclude.indexOf(filename) === -1)))
      return dirname(filename);
  }
  throw new Error('Could not resolve directory');
}

function fixup(str) {
  return stripBOM(str.toString()).replace(/\r/g, '');
}
function stripBOM(str){
  return 0xFEFF == str.charCodeAt(0)
    ? str.substring(1)
    : str;
}