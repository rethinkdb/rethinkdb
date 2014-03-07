var rfile = require('rfile');
var minify = require('uglify-js').minify;

module.exports = ruglify;
function ruglify(path, options) {
  options = options || {};
  options.exclude = [__filename];
  return minify(rfile.resolve(path, options), options).code;
}