# JavaScript Driver

This package is the officially supported driver for querying a
RethinkDB database from a JavaScript application.  It is designed to
be run in Node.js but also has an alternate http connection type that
is used by the RethinkDB web ui.  The http connection type is not
intended to be used by applications, since it isn't secure.
Ultimately, the http connection type will move into the web ui code
and out of the driver.

Check out
[rethinkdb.com/api/javascript](http://www.rethinkdb.com/api/javascript)
for documentation and examples of using this driver.

## Get and compile protobuf definitions

To get and compile protobuf definitions, you should run the `npm run get-proto`
script. This will download the proto file from rethinkdb's next branch and
convert it by `scripts/convert_protofile.py`.

## Compile coffee script

We are using a pretty outdated coffee script version right now, but to
mitigate compilation issues, you can use the `npm run compile` command. It
will compile the coffee script files for you.

## Build the package

Building the package combines getting proto file and compiling coffee script
source files. To build RethinkDB JavaScript driver run `npm run build`.

## Publishing

You can use NPM's default toolchain for publishing (`npm publish`), but please try some scenarios
with the freshly built driver before going forward. To test the driver, run at least
something like this after executing `node` command.

```javascript
var r = require('./dist/rethinkdb');
var db = 'test';
var table = 'testtable'

r.connect({db}, function(err, conn) {
    r.db(db).tableCreate(table).run(conn, function() {
        r.db(db).tableList().run(conn, console.log);
    });
});
```

To publish the package run `npm publish dist` with the appropriate credentials.
