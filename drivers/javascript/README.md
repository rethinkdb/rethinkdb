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

## Creating the proto-def.js

To build the JavaScript driver, you'll need to first generate the
definitions file `proto-def.js`. To do that from the `drivers`
directory, you can do:

```bash
$ ./convert_protofile -l javascript \
   -i ../src/rdb_protocol/ql2.proto \
   -o ./javascript/proto-def.js
```
