Query Language Tests
--------------------

This folder contains tests for the three official drivers:

* Javascript (js)
* Python (py)
* Ruby (rb)

There are three types of tests:

* Polyglot tests test the query language
* Connection tests test the connection API
* Cursor tests test the cursor API

The test framework is written in python.

### Dependencies

Rethinkdb and all its drivers need to be built to run all the tests.
Additionally, the javascript tests depend on the mocha test framework.

```
make -C ../..
sudo npm install -g mocha
```

### Running the tests

`make` runs all the tests except for `make attach`.

All the tests except for `make attach` launch their own instance of rethinkdb.
The server logs are stored in the `run/` folder. By default, the test suite looks
for a rethinkdb executable in `../../build/release`. This path can be changed by
passing the `BUILD_DIR=<path>` argument to make.

When the tests fail, they return a non-zero exit code. Make will exit as soon as a
test fails. To always run all the tests, run make with the the `-k` option.

### Language tests

The tests can be run for a specific language.

* `make py`
* `make js`
* `make rb`

### Polyglot tests

There are five ways to run the polyglot tests.

* `make polyglot` runs all the polyglot tests.
* `make js_polyglot`
* `make py_polyglot`
* `make rb_polyglot`
* `make attach` runs the javascript polyglot tests against a running server.

The tests can be customized by changing variables on the make command line.

* `TEST=<regexp>` to limit which tests are run. `make list` gives a list of tests that can be specified here.
* `PORT_OFFSET=<offset>` to attach to a rethinkdb instance with the corresponding port offset.
* `DRIVER_PORT=<port>` to attach to a different driver port.
* `CLUSTER_PORT=<port>` to attach to a different cluster port.
* `DEFAULT_LANGUAGE=<lang>` to specify what language tests to run when attaching.
* `SHARD=0|1` set to 1 to test with sharding.


### Connection tests

* `make connect`
* `make js_connect`
* `make py_connect`

* `TEST_DEFAULT_PORT=0|1` set to 1 if no rethinkdb server is running on the default ports.

### Cursor tests

* `make cursor`
* `make py_cursor`
* `make js_cursor`
