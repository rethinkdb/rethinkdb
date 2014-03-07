var resolve = require('../');
resolve('../', { filename: __filename }, function(err, path) {
    console.log(path);
});
