var browserify = require('../');
var vm = require('vm');
var through = require('through');
var test = require('tap').test;

var fs = require('fs');
var sources = {
    1: fs.readFileSync(__dirname + '/entry/main.js', 'utf8'),
    2: fs.readFileSync(__dirname + '/entry/one.js', 'utf8'),
    3: fs.readFileSync(__dirname + '/entry/two.js', 'utf8')
};

var deps = {
    1: { './two': 3, './one': 2 },
    2: {},
    3: {}
};

test('custom packer', function (t) {
    t.plan(7);
    
    var b = browserify({
        entries: [ __dirname + '/entry/main.js' ],
        pack: function () {
            return through(function (row) {
                t.equal(sources[row.id], row.source);
                t.deepEqual(deps[row.id], row.deps);
                this.queue(row.id + '\n');
            });
        }
    });
    b.bundle(function (err, src) {
        t.equal(src, '1\n2\n3\n');
    });
});
