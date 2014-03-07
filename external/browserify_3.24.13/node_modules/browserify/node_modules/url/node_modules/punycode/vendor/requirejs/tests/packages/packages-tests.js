require({
        baseUrl: require.isBrowser ? "./" : "./packages/",
        paths: {
            'alpha/replace' : 'replace'
        },
        packagePaths: {
            'pkgs': [
                'alpha',
                {
                    name: 'beta',
                    location: 'beta/0.2/scripts',
                    main: 'beta'
                },
                'dojox/chair',
                {
                    name: 'dojox/table',
                    main: 'table'
                }
            ]
        },
        packages: [
            {
                name: 'bar',
                location: 'bar/0.4',
                main: 'scripts/main'
            },
            {
                name: 'foo',
                location: 'foo/lib'
            },
            {
                name: 'funky',
                main: 'index.js'
            },
            {
                name: 'baz',
                location: 'baz/lib',
                main: 'index'
            },
            {
                name: 'dojox/window',
                location: 'dojox/window',
                main: 'window'
            }
        ]
    },
       ["require", "alpha", "alpha/replace", "beta", "beta/util", "bar", "baz",
        "foo", "foo/second", "dojox/chair", "dojox/table", "dojox/door", "dojox/window/pane",
        "dojox/window", "dojox/table/legs", "funky"],
function(require,   alpha,   replace,         beta,   util,        bar,   baz,
         foo,   second,       chair,         table,         door,         pane,
         window,         legs,               funky) {
    var dataUrl = require.toUrl('foo/../data.html');
    doh.register(
        "packages",
        [
            function packages(t){
                t.is("alpha", alpha.name);
                t.is("fake/alpha/replace", replace.name);
                t.is("beta", beta);
                t.is("beta/util", util.name);
                t.is("bar", bar.name);
                t.is("0.4", bar.version);
                t.is("baz", baz.name);
                t.is("0.4", baz.barDepVersion);
                t.is("foo", baz.fooName);
                t.is("baz/helper", baz.helperName);
                t.is("foo", foo.name);
                t.is("alpha", foo.alphaName);
                t.is("foo/second", second.name);
                t.is((require.isBrowser ? "./foo/lib/../data.html" : "./packages/foo/lib/../data.html"), dataUrl);
                t.is('dojox/chair', chair.name);
                t.is('dojox/chair/legs', chair.legsName);
                t.is('dojox/table', table.name);
                t.is('dojox/chair', table.chairName);
                t.is('dojox/table/legs', legs.name);
                t.is('dojox/door', door.name);
                t.is('dojox/window/pane', pane.name);
                t.is('dojox/window', window.name);
                t.is('dojox/window/pane', window.paneName);
                t.is('funky', funky.name);
                t.is('monkey', funky.monkeyName);
            }
        ]
    );
    doh.run();
});
