Test-Runner Tests
--------------------

`test-runner` has traditionally been used only for running tests on our drivers. The long-term plan (issue #3442) is to have it take over all testing.

It recursively scans the folder it is in to find files with one of four extensions:

* `.mocha` - run using the [Mocha](https://mochajs.org) JavaScript testing framework
* `.httpbin` - have a [httpbin](http://httpbin.org) server running while up with additional ENV variables: `HTTPBIN_HOST`, `HTTPBIN_HTTPBIN_PORT`, `HTTPBIN_HTTPCONTENT_PORT`, and `HTTPBIN_HTTPS_PORT`. `js` versions of these are additionally run through Mocha.
* `.yaml` - these are translated into all of the interpreter languages and run with our language "drivers" (`./drivers/driver.*`)
* `.test` - run normally, through an interpreter if specified (see below) or as a command-line executable otherwise

Each test is named for its relative path from this folder, with the exception that the `./src` folder is translated in the names to `polyglot`. For example:

`./connections/connection.py.test` results in the name `connections/connection.py2.6` for Python2.6
`./src/meta/composite.py.yaml` gets `polyglot/meta/composite.py2.6` for Python2.6

### Interpreters

Each test can specify what interpreter (e.g.: Ruby) it is written in using short codes (`js`, `py`, `rb`, `jrb`). If no interpreter is specified then the test is run as a command-line executable, but most cases should specify an interpreter. The interpreters to use are specified in a comma-separated list (e.g.: `grey.rb1.9,rb2.0.test`). Additionally single-ended ranges can be created using the `+` symbol either before or after the interpreter (e.g.: `+py2.7` to mean `[py2.6, py2.7]` or `py3+` to mean all versions of Python3.x).

For tests that are testing server-side behavior adding `_one` to the interpreter will run them for only the first version of the interpreter found.

The supported interpreters:

* Javascript (js)
* Python (py)
* Ruby (rb)
* JRuby (jrb)

Examples:

* `red.test` - run a test named `red` without an interpreter
* `green.py.test` - run a test named `green` for all selected versions of Python
* `blue.mocha` - run the `blue` test through the Mocha framework for all selected versions of Node
* `orange.py3.yaml` - run the `orange` test in all versions of Python 3
* `yellow.+py2_7.test` - run `yellow` tests for Python2.6 and Python2.7
* `purple.js_one.mocha` - run a Mocha test through only the first version of Node selected
* `grey.rb1.9,rb2_one.test` - run `grey` for both Ruby1.9 and the first Ruby2.x version selected

### Dependencies

Before all the tests will work, you must `./configure` and build RethinkDB, install node.js, as well as install the node.js mocha test framework.

```
make -C ../..
sudo npm install -g mocha
```

HTTP testing requires the `twisted` library for Python with OpenSSL support. Many systems have these installed, but if not they can be installed using `pip` (make sure you install it for correct Python version):

```
sudo pip install twisted
sudo pip install pyopenssl
```

And for testing all of our driver variants you also need the `tornado` and `gevent` libraries for Python, and `eventmachine` for Ruby.

```
sudo pip install tornado
sudo pip install gevent
sudo gem install eventmachine
```

All of these last group need to be installed for every version of Python or Ruby you wish to test them in. Python 3.0-3.2 are not supported by those projects.

### Basic Usage

`./test-runner` runs all the tests for the first version of Python, Ruby, and Node it can find.

Specific tests, or groups of tests, can be specified with regular expressions as arguments. Multiple groups of tests can be run with multiple arguments.

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

* `-d`/`--driver-port` can be used to specify an already-running server to use rather than instances started for the tests. Not all tests support this. A hostname can also be supplied in the format `hostname:port`, otherwise `localhost` is assumed.

* `-t`/`--table` specifies an existing table to use on user-supplied servers to use at the primary table. Not all tests support this.

* `-o`/`--output-dir` saves the console output both from the test and any servers managed by the test system
(i.e.: excluding user-supplied servers) to a folder in the specified directory. The server files for any failing tests are also collected in these folders.

* `--scratch-dir` puts all of the temporary files into the selected folder, including the database data files.
This can be useful in testing RethinkDB on different disks.

* `--clean` deletes the contents of the output directory before running. Only meaningful if `-o`/`--output-dir`
is specified.

* `-v`/`--verbose` lets the test's output go straight to stderr/stdout. This makes the tests run serially and
also disables saving test output from any output directory.

* `-b`/`--server-binary` uses the specified `rethinkdb` binary rather than automatically selecting the most recently built one.

* `-s`/`--shards` causes the default table to be sharded into the provided number of shards before each test
starts. Tests that create their own tables will not be affected by this.

* `-j`/`--jobs` runs multiple tests simultaneously. This defaults to 2.

There may be other options that are documented with the `-h/--help` command line option.

### Language tests

The tests can be run for a specific language using the `-i`/`--interpreter` flag.

* `./test-runner -i js`
* `./test-runner -i py`
* `./test-runner -i rb`

By default a single version of those languages are chosen based on what versions can be found. Specific interpreter versions can be chosen by adding the version number to the language suffix.

* `./test-runner -i py2.7`
* `./test-runner -i py3.4`

Multiple languages/language versions can be chosen simultaneously with multiple flags.

* `./test-runner -i py2.7 -i py3 -i rb`

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
