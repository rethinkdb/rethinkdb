var http = require('../../');
var JSONStream = require('JSONStream');

http.get({ path : '/data.json' }, function (res) {
    var parser = JSONStream.parse([ true ]);
    res.pipe(parser);
    
    parser.on('data', function (msg) {
        var div = document.createElement('div');
        div.textContent = JSON.stringify(msg);
        document.body.appendChild(div);
    });
});
