var resolve = require('../');
resolve('fs', null, function(err, path) {
    console.log(path);
});
