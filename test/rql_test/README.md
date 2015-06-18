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

Before the tests will work, you must `./configure` and build RethinkDB. Additionally, Python's YAML library
and the node.js mocha test framework need to be installed.

```
make -C ../..
sudo easy_install pyyaml
sudo npm install -g mocha
```

### Basic Usage

`./test-runner` runs all the tests.

Specific tests, or groups of tests, can be specified with regular expressions as arguments. Multiple groups of
tests can be run with muitple arguments.

`./test-runner connections`

`./test-runner polyglot/math`

`./test-runner connections/connection polyglot/regression/26*`

Tests can fail when they return non-zero exit codes, or timeout when they exceed 300 seconds. Testing will
continue and the output (STDERR and STDOUT) of that test will be printed to the screen. A summary of failing
tests is printed at the completion of testing.

By default the server data files are created in a hidden folder in the current working directory, and are deleted
at exit.

### Command line Options

* `-l`/`--list` lists all of the tests.

* `-i`/`--interpreter` specifies the drivers to use. See the "Language tests" section for more detail.

* `-c`/`--cluster-port` and `-d`/`--driver-port` can be used to specify an already-running server to use rather than the default which is to startup one or more servers. Not all tests support this.

* `-t`/`--table` specifies an existing table to use on user-supplied servers to use at the primary table. Not all tests support this.

* `-o`/`--output-dir` saves the console output both from the test and any servers managed by the test system
(i.e.: excluding user-supplied servers) to a folder in the specified directory. The server files for any failing tests are also collected in these folders.

* `--scratch-dir` puts all of the temporary files into the selected folder, including the database data files.
This can be useful in testing RethinkDB on different disks.

* `--clean` delete the contents of the output directory before running. Only meaningful if `-o`/`--output-dir`
is specified.

* `-v`/`--verbose` lets the test's output go straight to stderr/stdout. This makes the tests run serially and
also disables saving test output from any output directory.

* `-b`/`--server-binary` use the specified `rethinkdb` binary rather than automatically selecting the most recently built one.

* `-s`/`--shards` causes the default table to be sharded into the provided number of shards before each test
starts. Tests that create their own tables will not be affected by this.

* `-j`/`--jobs` runs multiple tests simultaneously. This defaults to 2.

There may be other options that are documented with the `-h/--help` command line option.

### Language tests

The tests can be run for a specific language using the `-i`/`--interpreter` flag.

* `./test-runner -i js`
* `./test-runner -i py`
* `./test-runner -i rb`

By default a single version of those languages are chosen based on what versions can be found. Specific
interpreter versions can be chosen by adding the version number to the language suffix with a dash.

* `./test-runner -i py-2.7`
* `./test-runner -i py-3.4`

Multiple languages/language versions can be chosen simultaneously with multiple flags.

* `./test-runner -i py-2.7 -i py3 -i rb`

### Polyglot tests

* `./test-runner polyglot` runs all the polyglot tests.
* `./test-runner polyglot -i js`
* `./test-runner polyglot -i py`
* `./test-runner polyglot -i rb`

### Connection tests

* `./test-runner connections/connection` runs all three versions of the tests
* `./test-runner -i js connections/connection`
* `./test-runner -i py connections/connection`
* `./test-runner -i rb connections/connection`

### Cursor tests

* `./test-runner run connections/cursor` runs all three versions of the tests
* `./test-runner -i js connections/cursor`
* `./test-runner -i py connections/cursor`
* `./test-runner -i rb connections/cursor`
