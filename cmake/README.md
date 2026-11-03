# Building RethinkDB

The server is built with CMake 3.24 or newer as ISO C++17
(`CMAKE_CXX_EXTENSIONS` is `OFF`). Out-of-source builds live in `build/`.

## Linux and macOS

```bash
cmake --preset debug \
  -DRETHINKDB_FETCH_BOOST=ON \
  -DRETHINKDB_FETCH_RE2=ON \
  -DRETHINKDB_FETCH_QUICKJS=ON \
  -DRETHINKDB_FETCH_GTEST=ON

cmake --build --preset debug --parallel

./build/debug/rethinkdb --version
./build/debug/rethinkdb-unittest
```

On Linux, also pass `-DRETHINKDB_FETCH_JEMALLOC=ON`. Release builds use the
`release` preset. Install with:

```bash
cmake --install build/release --prefix /usr/local
```

`DESTDIR` is honored. `RETHINKDB_STRIP_ON_INSTALL=ON` strips the installed
binary.

OpenSSL, zlib, curl, and Protobuf are found with `find_package`. Set
`RETHINKDB_FETCH_OPENSSL`, `RETHINKDB_FETCH_ZLIB`, `RETHINKDB_FETCH_CURL`, or
`RETHINKDB_FETCH_PROTOBUF` to build the pinned versions instead. QuickJS is
always the RethinkDB fork and is always fetched. Boost is 1.84 because 1.60
does not compile as C++17. RE2 stays at the 2015-11-01 pin. Google Test stays
at 1.8.1 and is compiled as C++11.

## Options

| Option | Default | Meaning |
| --- | --- | --- |
| `RETHINKDB_ALLOCATOR` | `jemalloc` on Linux, `system` elsewhere | `jemalloc`, `tcmalloc`, or `system` |
| `RETHINKDB_ALLOW_WARNINGS` | `ON` | `OFF` adds `-Werror` |
| `RETHINKDB_SYMBOLS` | `ON` | Debug symbols. Linux also links `-rdynamic` |
| `RETHINKDB_SPLIT_SYMBOLS` | `OFF` | Strip the binary and write `.debug` or a `.dSYM` |
| `RETHINKDB_STATIC` | `OFF` | Mostly-static Linux link |
| `RETHINKDB_BUILD_PORTABLE` | `OFF` | Link libgcc statically on Linux |
| `RETHINKDB_VERSION` | empty | Packaging version override |
| `BUILD_TESTING` | `ON` | Build `rethinkdb-unittest` |

Feature switches, all default `OFF`: `RETHINKDB_NO_EPOLL`,
`RETHINKDB_NO_EVENTFD`, `RETHINKDB_THREADED_COROUTINES`, `RETHINKDB_VALGRIND`,
`RETHINKDB_LEGACY_LINUX`, `RETHINKDB_LEGACY_GCC`, `RETHINKDB_FULL_PERFMON`,
`RETHINKDB_CORO_PROFILING`, `RETHINKDB_RQL_ERROR_BT`, `RETHINKDB_JSON_SHORTCUTS`,
`RETHINKDB_DISABLE_BREAKPOINTS`, `RETHINKDB_COVERAGE`.

`RETHINKDB_VALGRIND` requires `RETHINKDB_ALLOCATOR=system`.
`RETHINKDB_COVERAGE` requires GCC.

## Other targets

- `cmake --build build/debug --target dist` writes
  `build/debug/packages/rethinkdb-<version>.tgz`. The tarball includes the
  fetched Boost, RE2, QuickJS, Google Test, and jemalloc sources, so those
  fetch options must have been enabled at configure time. `RETHINKDB_BUILD_PORTABLE`
  also copies the Protobuf tree.
- `cmake --build build/debug --target build-deb-src` and `build-deb` wrap
  `debuild` / `dpkg-buildpackage`. The binary package builds the server only
  (`BUILD_TESTING=OFF`). Compile jobs come from `cmake/parallel-jobs.sh`, which
  caps `dpkg-buildpackage --jobs=auto` to what fits in RAM.
- `cmake --build build/release --target build-osx` builds the disk image on macOS.
- `cpack` in the build directory builds a binary TGZ, and DEB/RPM or DragNDrop
  when that generator applies.
- `generate-headers` rewrites the checked-in serialization headers.
- `web-assets` rewrites `src/gen/web_assets.cc`. Set `RETHINKDB_WEB_ASSETS_DIR`
  to the admin UI tree first. A normal build uses the committed file.

## Style

`scripts/check_cmake.sh` checks `CMakeLists.txt` and the files under `cmake/`
with [cmakelint](https://github.com/cmake-lint/cmake-lint) 1.4.3. Settings are
in `.cmakelintrc`. Install the checker with:

```bash
python -m pip install 'cmakelint==1.4.3'
```

CI runs that script in the preflight job, before the build.

## FreeBSD

Install `git`, `gcc`, `gmake`, `cmake`, and `python3`, then configure with the
system compiler you intend to use:

```bash
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug \
  -DRETHINKDB_FETCH_BOOST=ON -DRETHINKDB_FETCH_RE2=ON \
  -DRETHINKDB_FETCH_QUICKJS=ON -DRETHINKDB_FETCH_GTEST=ON

cmake --build build/debug --parallel
```

QuickJS is built with the same C compiler CMake selected.

## Windows

See [WINDOWS.md](../WINDOWS.md).
