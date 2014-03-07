var resolve = require('../');
var parent = { filename: __dirname + '/custom/file.js' };
resolve('./main.js', parent, function(err, path) {
    console.log(path);
});
