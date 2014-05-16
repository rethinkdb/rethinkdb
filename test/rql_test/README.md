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

Rethinkdb needs to have `./configure` run, and been built before these tests will work. 
Additionally, the javascript tests require that both node.js and the mocha test framework be installed.

```
make -C ../..
sudo npm install -g mocha
```

### Basic Usage

`./test_runner` runs all the tests.

`./test_runner --list` lists all of the tests.

`./test_runner --clean` cleans the `run` folder where the runtime data is put.

Specific tests, or groups of tests, can be spcified with regular expressions as arguments. Additionally the 
`-i/--interpreter` flag can be used one or more times to specifiy the drivers to use. 

Unless you specifiy the `--cluster-port` and `--driver-port` one or more instances of RethinkDB will be started for
testing. Some of the tests (e.g.: the connection tests) will additionally launch multiple copies of the server.

The tests write out data to a `run` folder that is created in the current working directory.

By default, the test suite uses the most recently built rethinkdb binary from ../../build/, but this can be changed by
passing the `-b` or `--build-dir` argument.

When the tests fail, they return a non-zero exit code. Testing will continue and the output (STDERR and STDOUT) of that
test will be printed to the screen. A summary of failing tests is printed at the completion of testing.

### Language tests

The tests can be run for a specific language.

* `./test_runner -i py`
* `./test_runner -i js`
* `./test_runner -i rb`

### Polyglot tests

There are five ways to run the polyglot tests.

* `./test_runner run polyglot` runs all the polyglot tests.
* `./test_runner run polyglot -i py`
* `./test_runner run polyglot -i js`
* `./test_runner run polyglot -i rb`

### Connection tests

* `./test_runner run connections/connection` run both the javascript and python versions of the tests
* `./test_runner run -i js connections/connection`
* `./test_runner run -i py connections/connection`

### Cursor tests

* `./test_runner run connections/cursor` run both the javascript and python versions of the tests
* `./test_runner run -i js connections/cursor`
* `./test_runner run -i py connections/cursor`

### Customization

The tests can be customized with command line options:

*  `-b/--build-dir` specifies the build directory to use
*  `-i/--interpreter` run only the given language, can be given multiple times for multiple languages
*  `-s/--shards` run with the given number of shards (defaults to 1). Not all tests support this.
*  `-v/--verbose` display all test output as it is running

There are more options that are documented with the `-h/--help` command line option.
