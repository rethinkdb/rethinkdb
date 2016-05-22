The RethinkDB build system
==========================

To build RethinkDB, run:

```
./configure --allow-fetch
make
```

The `--allow-fetch` option tells `make` to fetch and build missing
dependencies.

The full list of options accepted by the configure script can be
listed with `./configure --help`. The full list of variables that can
be passed to `make` are documented in `mk/defaults.mk`. The possible
targets are described below.

Development
-----------

Useful targets and variables during development.

* `make clean`: Remove the entire `build/` directory.

* `make SHOW_COUNTDOWN=0`: Do not prefix `make` output with the rule
  count (such as `[3/301]`).

* `make VERBOSE=1`: Display each command being executed and their
  output.

* `make -j <n>`: Build in parallel. A good default for `<n>` is the
  amount of cores plus one.


Server
------

The server may depend on some of the fetched dependencies. Accessing
its web UI also requires the web assets.

* `make rethinkdb`: Only build the server executable.

* `make tags`, `make etags`: Generate tag files used by vim and emacs.

* `make ALLOW_WARNINGS=1`: Do not fail if the C++ compiler generates a
  warning, by disabling `-Werror`.

* `./configure CXX=... CXXFLAGS=...`: Build with a different compiler
  or flags.

* `./configure --ccache`: Speed up repeated builds using `ccache`.

* RethinkDB employees can use `./configure CXX=recc` to speed up the
  build. The executable, in `newton:/usr/local/bin/recc`, wraps both
  `ccache` and `distcc` to distribute the build across our server
  infrastructure and cache the result.

* There is a `check-syntax` target that can be used with emacs'
  flymake-mode.

* `make SYMBOLS=0`: Exclude debugging symbols.

* `make SPLIT_SYMBOLS=1`: Strip the executable and generate a symbols file.

* `make DEBUG=1`: Build in debug mode.

* When building on older distributions or porting to different
  platforms, these `make` options can also be useful:
  `THREADED_COROUTINES=1` `NO_EVENTFD=1` `NO_EPOLL=1`
  `BUILD_PORTABLE=1` or `LEGACY_LINUX=1`


Web assets
----------

* `make web-assets`: Build only the web assets.

* If there is a `precompiled/web` folder, the web assets will not be
  rebuilt unless the `--disable-precompiled-web` flag is passed to
  `./configure`

* The web assets are compiled into the executable. To speed up
  development time and avoid rebuilding the executable, the
  `--web-static-directory` option can be used. For example:
  `build/release/rethinkdb --web-static-directory build/web_assets`

Drivers
-------

* `make ruby-driver`: Build the Ruby driver

* `make python-driver`: Build the Python driver.

* `make js-driver`: Build the JavaScript driver.

* `make drivers`: Build all the drivers.


Test
---

* `make unit`: Build and run the unit tests.

* `make test`: Run the unit tests, reql tests and integration
  tests. The `TEST` variables determines which tests to run. See
  `test/run -h` for more documentation.


Dependencies
---

* `make support` or `make support-<dep>`: Build the dependencies that
  have not been built yet.

* `make fetch` or `make fetch-<dep>`: Fetch the dependencies that have
  not yet been fetched.

* `make build-<dep>`: Rebuild a given dependency.

* `make clean-<dep>`: Clean the build directory of a given dependency.

* `install-include-<dep>`: Copy the include files from the given
  dependency.

* `VERIFY_FETCH_HASH=0`: Don't verify the hash of fetched archives.


Installation
---

* `./configure --prefix`: Set the prefix folder used by `make install`.

* `make install`: Install RethinkDB to the prefix. If the `DESTDIR`
  variable is passed, install the files there. If `STRIP_ON_INSTALL=1`
  is passed, also strip the executable.

* `make install-binaries`: Only install the server executable.


Packaging
---

Packaging is usually automated by the BuildBot instance running on
`dr-doom:8010` in our intranet.

* `make build-deb-src UBUNTU_RELEASE=<name> PACKAGE_BUILD_NUMBER=<n>`:
  Build a package for the given Ubuntu version.

* `make build-osx`: Build a `dmg` installer for OS X. Jenkins always
  builds the `dmg` on OS X 10.7.5.

* `make dist`: Build a source distribution.

* The CentOS RPM are built with `scripts/build-rpm.sh`.


The build system
----------------

Some of the flags and targets can be used to debug the build system
itself.

* make `var-<VAR>`: Print the value of the given variable.

* `make -B`: Force rebuilding all files.

* `make -d`: Tell `make` to dump a lot of debug information.

* `make IGNORE_MAKEFILE_CHANGES=1`: Do not rebuild files if the
  makefiles have changes.

* `make SHOW_BUILD_REASON=1`: Show which file triggered each rule.

* `make dump-db` or `make -p`: Dump the `make` database.

* `make command-line`: Print the real `make` command line used to
  build RethinkDB. On some old, unpatched versions of GNU make this is
  required to use `-j`.

* `make debug-count`: Debug the count feature.

To be able to invoke make from a subdirectory, a Makefile must exist.
It usually looks like this:

```
OVERRIDE_GOALS ?= default-goal=<default target for this directory>
TOP := <relative path to the top of the rethinkdb source tree>
include $(TOP)/Makefile
```
