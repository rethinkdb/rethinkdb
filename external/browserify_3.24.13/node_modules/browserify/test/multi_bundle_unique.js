var browserify = require('../');
var vm = require('vm');
var fs = require('fs');
var path = require('path');
var test = require('tap').test;
var prelude = fs.readFileSync(path.join(__dirname, 'multi_bundle', '_prelude.js'), 'utf8');

test('unique require', function (t) {
    t.plan(6);

    var core = browserify();
    core.require(__dirname + '/multi_bundle/b.js', { expose: 'b' });

    var app = browserify([__dirname + '/multi_bundle/a.js']);
    // inform this bundle that b exists in another bundle
    app.external(__dirname + '/multi_bundle/b.js');

    core.bundle({ externalRequireName: 'unique_require', prelude: prelude }, function (err, src) {
        var c = {
            console: console,
            t : t,
            baton: {
                times: 0
            }
        };

        // loading core will cause no require to run
        vm.runInNewContext(src, c);
        t.equal(c.baton.times, 0);

        // loading the app will require
        app.bundle({ prelude: prelude }, function (err, src) {
            vm.runInNewContext(src, c);

            // b required for the first time
            t.equal(c.baton.times, 1);

            // running the file again
            // because it is using the same b, no reloading
            vm.runInNewContext(src, c);

            // b should not have been required again
            // because it was part of the core bundle
            t.equal(c.baton.times, 1);
            
            t.equal(c.unique_require('b'), 'foo');
        });
    });
});

