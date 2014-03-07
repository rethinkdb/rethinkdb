var resolve = require('../');
var parent = { filename: __dirname + '/skip/main.js' };
resolve('tar', parent, function(err, path) {
    console.log(path);
});
