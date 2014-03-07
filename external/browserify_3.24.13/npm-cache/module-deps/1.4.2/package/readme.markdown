# module-deps

walk the dependency graph to generate json output that can be fed into
[browser-pack](https://github.com/substack/browser-pack)

[![build status](https://secure.travis-ci.org/substack/module-deps.png)](http://travis-ci.org/substack/module-deps)

# example

``` js
var mdeps = require('module-deps');
var JSONStream = require('JSONStream');

var stringify = JSONStream.stringify();
stringify.pipe(process.stdout);

var file = __dirname + '/files/main.js';
mdeps(file).pipe(stringify);
```

output:

```
$ node example/deps.js
[
{"id":"/home/substack/projects/module-deps/example/files/main.js","source":"var foo = require('./foo');\nconsole.log('main: ' + foo(5));\n","entry":true,"deps":{"./foo":"/home/substack/projects/module-deps/example/files/foo.js"}}
,
{"id":"/home/substack/projects/module-deps/example/files/foo.js","source":"var bar = require('./bar');\n\nmodule.exports = function (n) {\n    return n * 111 + bar(n);\n};\n","deps":{"./bar":"/home/substack/projects/module-deps/example/files/bar.js"}}
,
{"id":"/home/substack/projects/module-deps/example/files/bar.js","source":"module.exports = function (n) {\n    return n * 100;\n};\n","deps":{}}
]
```

and you can feed this json data into
[browser-pack](https://github.com/substack/browser-pack):

```
$ node example/deps.js | browser-pack | node
main: 1055
```

# usage

```
usage: module-deps [files]

  generate json output from each entry file

```

# methods

``` js
var mdeps = require('module-deps')
```

## mdeps(files, opts={})

Return a readable stream of javascript objects from an array of filenames
`files`.

Each file in `files` can be a string filename or a stream.

Optionally pass in some `opts`:

* opts.transform - a string or array of string transforms (see below)

* opts.transformKey - an array path of strings showing where to look in the
package.json for source transformations. If falsy, don't look at the
package.json at all.

* opts.resolve - custom resolve function using the
`opts.resolve(id, parent, cb)` signature that
[browser-resolve](https://github.com/shtylman/node-browser-resolve) has

* opts.filter - a function (id) to skip resolution of some module `id` strings.
If defined, `opts.filter(id)` should return truthy for all the ids to include
and falsey for all the ids to skip.

* opts.packageFilter - transform the parsed package.json contents before using
the values. `opts.packageFilter(pkg)` should return the new `pkg` object to use.

* opts.noParse - an array of absolute paths to not parse for dependencies. Use
this for large dependencies like jquery or threejs which take forever to parse.

* opts.cache - an object mapping filenames to file objects to skip costly io

* opts.packageCache - an object mapping filenames to their parent package.json
contents for browser fields, main entries, and transforms

# transforms

module-deps can be configured to run source transformations on files before
parsing them for `require()` calls. These transforms are useful if you want to
compile a language like [coffeescript](http://coffeescript.org/) on the fly or
if you want to load static assets into your bundle by parsing the AST for
`fs.readFileSync()` calls.

If the transform is a function, it should take the `file` name as an argument
and return a through stream that will be written file contents and should output
the new transformed file contents.

If the transform is a string, it is treated as a module name that will resolve
to a module that is expected to follow this format:

``` js
var through = require('through');
module.exports = function (file) { return through() };
```

You don't necessarily need to use the
[through](https://github.com/dominictarr/through) module to create a
readable/writable filter stream for transforming file contents, but this is an
easy way to do it.

When you call `mdeps()` with an `opts.transform`, the transformations you
specify will not be run for any files in node_modules/. This is because modules
you include should be self-contained and not need to worry about guarding
themselves against transformations that may happen upstream.

Modules can apply their own transformations by setting a transformation pipeline
in their package.json at the `opts.transformKey` path. These transformations
only apply to the files directly in the module itself, not to the module's
dependants nor to its dependencies.

# install

With [npm](http://npmjs.org), to get the module do:

```
npm install module-deps
```

and to get the `module-deps` command do:

```
npm install -g module-deps
```

# license

MIT
