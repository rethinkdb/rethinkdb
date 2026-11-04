# Building and packaging RethinkDB

Build the server, install it, or produce a source archive, Debian package, RPM,
macOS disk image, or Windows executable. The build uses CMake 3.24 or newer.
The C++ sources are ISO C++17 (`CMAKE_CXX_STANDARD` is 17 and
`CMAKE_CXX_EXTENSIONS` is `OFF`). C sources are C11. Builds are out of source,
under `build/`.

## Before you build

Install CMake 3.24 or newer, a C++17 compiler, Python 3, `m4`, and `patch`.
`patch` applies the fixes for fetched dependencies. A Git checkout uses
`scripts/gen-version.sh` to compute the version, so Git must be available
there. A source archive that already contains `VERSION.OVERRIDE` does not need
Git for the version string.

These dependencies are fetched by default:

| Dependency | Pin | Option |
| --- | --- | --- |
| Boost headers | 1.84.0 | `RETHINKDB_FETCH_BOOST` |
| RE2 | 2015-11-01 | `RETHINKDB_FETCH_RE2` |
| QuickJS | RethinkDB fork `95d6dcf6358a74e9cbe04eade7c383f43ba306cb` | `RETHINKDB_FETCH_QUICKJS` |
| Google Test | 1.8.1, compiled as C++11 | `RETHINKDB_FETCH_GTEST` |
| jemalloc | 5.2.1, Linux only | `RETHINKDB_FETCH_JEMALLOC` |

QuickJS is the RethinkDB fork and has no system package. Configure fails if
`RETHINKDB_FETCH_QUICKJS` is `OFF`.

On Unix, OpenSSL, zlib, curl, and Protobuf come from the system. Set
`RETHINKDB_FETCH_OPENSSL`, `RETHINKDB_FETCH_ZLIB`, `RETHINKDB_FETCH_CURL`, or
`RETHINKDB_FETCH_PROTOBUF` to build the pinned copies instead. On Windows those
four options default to `ON`.

Fetched archives are stored in `.cache/fetchcontent`. The first configure needs
network access when an archive is not already there. A release compile of
fetched Protobuf can use several GiB for one compiler process. On Linux, cap
the job count:

```bash
jobs=$(sh cmake/parallel-jobs.sh)
cmake --build --preset release --parallel "${jobs}"
```

`cmake/parallel-jobs.sh` reads available memory and prints a job count. It also
accepts `RETHINKDB_NPROC`, `RETHINKDB_MEM_AVAILABLE_KB`, and
`DEB_BUILD_OPTIONS=parallel=N`. `parallel=N` is a ceiling. It does not raise
the count above what fits in RAM.

## Build on macOS

The minimum deployment target is macOS 12.0. Configure raises an older
`CMAKE_OSX_DEPLOYMENT_TARGET` to 12.0.

1. Install the Xcode Command Line Tools, CMake, and Protobuf. Homebrew OpenSSL
   is found under `/opt/homebrew/opt/openssl@3` or
   `/usr/local/opt/openssl@3`.

   ```bash
   xcode-select --install
   brew install cmake protobuf openssl@3
   ```

   To build the pinned Protobuf instead of the Homebrew copy, add
   `-DRETHINKDB_FETCH_PROTOBUF=ON` to the configure command below.

2. Configure and build the debug server.

   ```bash
   cmake --preset debug
   cmake --build --preset debug --parallel
   ```

3. Run the server and the unit tests.

   ```bash
   ./build/debug/rethinkdb --version
   ./build/debug/rethinkdb-unittest
   ctest --test-dir build/debug --output-on-failure
   ```

4. For a release build, repeat the configure and build with the `release`
   preset. The binary is `build/release/rethinkdb`.

The allocator on macOS is the system allocator.

## Build on Debian and Ubuntu

1. Install the compiler, CMake, and the system libraries.

   ```bash
   sudo apt-get update
   sudo apt-get install build-essential cmake python3 m4 patch git \
     libssl-dev zlib1g-dev libcurl4-openssl-dev \
     protobuf-compiler libprotobuf-dev
   ```

   `protobuf-compiler` and `libprotobuf-dev` are required when
   `RETHINKDB_FETCH_PROTOBUF` stays at its default, `OFF`. jemalloc 5.2.1 is
   fetched. `libjemalloc-dev` is required only when you set
   `RETHINKDB_FETCH_JEMALLOC=OFF`.

2. Configure, build, and run the debug server.

   ```bash
   cmake --preset debug
   cmake --build --preset debug --parallel
   ./build/debug/rethinkdb --version
   ./build/debug/rethinkdb-unittest
   ```

3. Build the release server with a job count that fits in RAM.

   ```bash
   cmake --preset release
   cmake --build --preset release --parallel "$(sh cmake/parallel-jobs.sh)"
   ./build/release/rethinkdb --version
   ```

## Build on RHEL, CentOS, Rocky, and Alma

These commands use `dnf` on RHEL, CentOS Stream, Rocky Linux, and AlmaLinux.
The `cmake` package must be version 3.24 or newer. Check with
`cmake --version` before configuring.

1. Install the compiler, CMake, and the system libraries.

   ```bash
   sudo dnf install -y gcc gcc-c++ make cmake python3 m4 patch git \
     openssl-devel zlib-devel libcurl-devel
   ```

   `protobuf-devel` and `protobuf-compiler` are in the CodeReady Builder
   repository, which is disabled by default. On Rocky, Alma, and CentOS Stream,
   enable it for this install:

   ```bash
   sudo dnf --enablerepo=crb install -y protobuf-devel protobuf-compiler
   ```

   On RHEL, the repository name includes the architecture:

   ```bash
   sudo dnf --enablerepo="codeready-builder-for-rhel-9-$(uname -m)-rpms" \
     install -y protobuf-devel protobuf-compiler
   ```

   That repository ships Protobuf 3.14. To build the pinned Protobuf 36.2
   instead, skip those two packages and configure with
   `-DRETHINKDB_FETCH_PROTOBUF=ON`. jemalloc is fetched unless
   `RETHINKDB_FETCH_JEMALLOC` is `OFF`.

2. Configure and build.

   ```bash
   cmake --preset debug
   cmake --build --preset debug --parallel
   ./build/debug/rethinkdb --version

   cmake --preset release
   cmake --build --preset release --parallel "$(sh cmake/parallel-jobs.sh)"
   ./build/release/rethinkdb --version
   ```

## Build on Windows

The Windows build uses the Visual Studio generator, the MSVC v141 toolset, and
the Windows 10 SDK 10.0.19041.0. The server links the static CRT (`/MT` in
Release, `/MTd` in Debug). Fetch defaults for Boost, RE2, QuickJS, Google Test,
Protobuf, OpenSSL, zlib, and curl are `ON`. The allocator stays the system
allocator.

1. Install Visual Studio 2022 with the desktop C++ workload, plus:

   - MSVC v141
   - Windows 10 SDK 10.0.19041.0
   - CMake 3.24 or newer

2. Install [Strawberry Perl](https://strawberryperl.com/) and put it on `PATH`.
   The fetched OpenSSL build runs Perl. A Cygwin `perl` earlier on `PATH`
   breaks that build.

3. Install [Premake 5](https://premake.github.io/) and put `premake5` on
   `PATH`. The fetched QuickJS build runs Premake.

4. Open the Visual Studio developer command prompt so `nmake` and `msbuild`
   are on `PATH`. The fetched OpenSSL, zlib, and curl builds use them.

5. Configure and build from the source root.

   ```bat
   cmake -S . -B build -G "Visual Studio 17 2022" -T v141 -A x64
   cmake --build build --config Release --parallel
   ```

   The explicit fetch options are not required. Windows defaults already select
   the pinned dependencies.

6. Run the executable.

   ```bat
   build\Release\rethinkdb.exe --version
   build\Release\rethinkdb-unittest.exe
   ```

   A Debug build writes `build\Debug\rethinkdb.exe` and
   `build\Debug\rethinkdb-unittest.exe`. `src/arch/windows_stub` is on the
   include path for Windows only.

`RETHINKDB_WINDOWS_TOOLSET` and `RETHINKDB_WINDOWS_SDK` select the toolset and
SDK used to build the fetched Windows dependencies. The defaults match the
generator command above.

## Build on FreeBSD

Install `git`, `gcc`, `gmake`, `cmake`, `python3`, and the system OpenSSL,
zlib, curl, and Protobuf development packages. Then configure with the
compiler you intend to use:

```bash
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug --parallel
./build/debug/rethinkdb --version
```

QuickJS is built with the same C compiler CMake selected. The allocator is the
system allocator. Pass `-DRETHINKDB_FETCH_PROTOBUF=ON` when the system
Protobuf is not installed. There is no FreeBSD preset.

## Install

`cmake --install` installs the server, its gzip-compressed man page, the
license at `share/doc/rethinkdb/copyright`, the SysV init script, the sample
configuration, and empty configuration and data directories.

```bash
sudo cmake --install build/release --prefix /usr/local
```

`DESTDIR` selects a staging root:

```bash
DESTDIR=/tmp/rethinkdb-stage cmake --install build/release
```

The default state directory is `<prefix>/var`. When the prefix is `/usr`, the
default is `/var`. Override it with `RETHINKDB_LOCALSTATEDIR`. Set
`RETHINKDB_STRIP_ON_INSTALL=ON` before configuring to strip the installed
binary. Set `RETHINKDB_NAMEVERSIONED=ON` to install the executable, man page, and doc
directory as `rethinkdb-<major>.<minor>`. Leave that option `OFF` when building
the macOS disk image. The image script looks for the installed name
`rethinkdb`.

## Package a source tarball

The `dist` target writes `build/<preset>/packages/rethinkdb-<version>.tgz`.
The archive contains the server sources, `VERSION.OVERRIDE`, the generated
protocol files, and the fetched sources recorded at configure time. `dist`
requires fetched Boost, RE2, QuickJS, Google Test, and jemalloc. With
`RETHINKDB_BUILD_PORTABLE=ON`, it also requires fetched Protobuf.

On Linux, jemalloc is already fetched by default. Google Test is fetched by
default except in the `package-linux` preset, which turns it off.

```bash
cmake --preset release -DRETHINKDB_BUILD_PORTABLE=ON -DRETHINKDB_FETCH_PROTOBUF=ON
cmake --build --preset release --target dist --parallel "$(sh cmake/parallel-jobs.sh)"
```

The archive is a source tree. It is not the generic CPack `package_source`
snapshot. Use `dist` for the source release.

## Package a Debian source package

`package-deb-src` is available when the build host is Linux. It depends on
`dist`, copies `packaging/debian` into that tree, and runs `debuild -S`. It
does not compile the server. `packaging/debian/rules` compiles the server when
that source package is built later, and it caps compile jobs with
`cmake/parallel-jobs.sh`.

Install `devscripts`, which provides `debuild`. The generated `debian/control`
also build-depends on `debhelper`, `fakeroot`, `m4`, `python3`,
`python-is-python3`, `cmake`, `g++`, `libcurl4-openssl-dev`, and
`libssl-dev`. A non-portable source package also
build-depends on `protobuf-compiler` and `libprotobuf-dev`.

`package-linux` disables Google Test, which `dist` requires. Turn it back on
for this target:

```bash
cmake --preset package-linux -DRETHINKDB_FETCH_GTEST=ON
cmake --build --preset package-deb-src --parallel "$(sh cmake/parallel-jobs.sh)"
```

Optional environment variables, as set by the CMake target or the shell:

| Variable | Default | Effect |
| --- | --- | --- |
| `UBUNTU_RELEASE` | empty | Appended to the Debian version and used as the changelog distribution |
| `DEB_RELEASE` | empty | Same role as `UBUNTU_RELEASE`; takes precedence when both are set |
| `PACKAGE_BUILD_NUMBER` | `1` | Added to the Debian version as `~<number>` |
| `SIGN_PACKAGE` | `0` | `1` lets `debuild` sign. Otherwise the command passes `-us -uc` |
| `BUILD_PORTABLE` | from the CMake option | `1` omits Protobuf from the source package build-depends |

The changelog distribution is `unstable` when neither release variable is set.
The source package is written around `build/package-linux/packages/dsc/`.

## Package a Debian binary package

`package-deb` is available on Linux. It builds `rethinkdb` and `rethinkdb_man`,
then runs CPack with the DEB generator. Install `dpkg-dev` and `file`. CPack uses
`file` and `dpkg-shlibdeps` to discover shared-library dependencies.

The `package-linux` preset is a Release build with tests off, symbols left in
the binary, `/usr`, `/etc`, and `/var`, and fetched Boost, RE2, QuickJS,
jemalloc, and Protobuf. OpenSSL, zlib, and curl stay on the system so the
package dependencies point at them. `RETHINKDB_BUILD_PORTABLE` is `ON`.

```bash
sudo apt-get install dpkg-dev file
cmake --preset package-linux
cmake --build --preset package-deb --parallel "$(sh cmake/parallel-jobs.sh)"
```

CPack writes the `.deb` files to `build/package-linux/packages/deb/`. When
symbols are enabled and the binary is not stripped by `RETHINKDB_SPLIT_SYMBOLS`
or `RETHINKDB_STRIP_ON_INSTALL`, CPack also writes a `.ddeb` debug-symbol
package. The package runs the scripts in `packaging/debian/` to create the
`rethinkdb` user and register the init script.

## Package an RPM

`package-rpm` is available on Linux. It uses the same `package-linux` build as
the Debian package and runs CPack with the RPM generator. Install `rpm-build`.
The preset leaves symbols in the binary, so CPack builds the debuginfo RPM and
`rpm-build` must be present.

```bash
sudo dnf install rpm-build
cmake --preset package-linux
cmake --build --preset package-rpm --parallel "$(sh cmake/parallel-jobs.sh)"
```

CPack writes the RPM files to `build/package-linux/packages/rpm/`.
`packaging/rpm/preinstall.sh` creates the `rethinkdb` system group and user.
CPack discovers shared-library requirements from the binary. The RPM version
replaces `-` in the version string with `_`. The release number is the Git
revision count from `scripts/gen-version.sh -r`, or `1` when that count is not
available.

## Package a macOS disk image

`package-osx` is available on macOS. It builds `rethinkdb` and `rethinkdb_man`,
stages `cmake --install`, and runs `packaging/osx/create_dmg.py`. The image
contains the installer package, the uninstaller, and the release-notes link.
CPack does not build this image.

Install the Xcode Command Line Tools. `pkgbuild`, `productbuild`, `hdiutil`,
`osacompile`, and `otool` come from those tools. The disk-image builder also
needs `pyobjc-framework-Quartz`:

```bash
python3 -m pip install pyobjc-framework-Quartz
```

The `package-osx` preset is a Release build with tests off. It fetches Boost,
RE2, QuickJS, and Protobuf, and installs the executable under `/usr/local`.

```bash
cmake --preset package-osx
cmake --build --preset package-osx --parallel
```

The image is `build/package-osx/packages/osx/rethinkdb.dmg`. Before the image
is built, `otool` must show that every linked library lives outside the build
directory. A library under `build/package-osx` fails the package. The preset
uses system OpenSSL, zlib, and curl, including Homebrew OpenSSL when CMake
finds it under `/opt/homebrew` or `/usr/local`.

## Package a Windows executable

The Windows deliverable is the executable produced by the Visual Studio build:

```text
build\Release\rethinkdb.exe
```

There is no MSI, NSIS, or WiX target. After the Windows configure in
[Build on Windows](#build-on-windows), CPack can archive the install tree as a
`.tar.gz`:

```bat
cmake --build build --config Release --target package
```

The archive is written under `build\packages\tgz`. `cpack` without `-G` uses
the same TGZ generator on every platform.

## Package an existing build

`cmake/package-deb.sh`, `cmake/package-rpm.sh`, and `cmake/package-osx.sh`
package a build that already contains `rethinkdb` and the man page. They do
not run configure or compile. The CMake targets above are the normal way to
build those prerequisites and then package them.

From a Linux `package-linux` build:

```bash
MODE=binary SOURCE_DIR="$PWD" BINARY_DIR=build/package-linux CONFIG=Release \
  bash cmake/package-deb.sh
BINARY_DIR=build/package-linux CONFIG=Release bash cmake/package-rpm.sh
```

`BINARY_DIR` is required. The directory must contain `CMakeCache.txt`,
`CPackConfig.cmake`, and the server binary. `CONFIG` selects a
multi-configuration output directory when
`<binary-dir>/<config>/rethinkdb` exists, and it is passed to CPack as `-C`.

From a macOS `package-osx` build, pass the build directory, install prefix,
binary directory, and configuration:

```bash
bash cmake/package-osx.sh build/package-osx /usr/local bin Release
```

For a Debian source package, `MODE=src` also requires `RETHINKDB_VERSION` and
the `dist` tree at `<binary-dir>/packages/rethinkdb-<version>`:

```bash
MODE=src RETHINKDB_VERSION="$(./scripts/gen-version.sh)" \
  SOURCE_DIR="$PWD" BINARY_DIR=build/package-linux \
  bash cmake/package-deb.sh
```

## Presets

`debug` and `release` are available on every host. The package presets are
hidden unless `hostSystemName` matches.

| Configure preset | Build directory | Host | Build type |
| --- | --- | --- | --- |
| `debug` | `build/debug` | any | Debug |
| `release` | `build/release` | any | Release |
| `package-linux` | `build/package-linux` | Linux | Release |
| `package-osx` | `build/package-osx` | Darwin | Release |

| Build preset | Configure preset | Target |
| --- | --- | --- |
| `debug` | `debug` | default `all` target |
| `release` | `release` | default `all` target |
| `package-deb` | `package-linux` | `package-deb` |
| `package-rpm` | `package-linux` | `package-rpm` |
| `package-deb-src` | `package-linux` | `package-deb-src` |
| `package-osx` | `package-osx` | `package-osx` |

`package-linux` sets `BUILD_TESTING=OFF`, `RETHINKDB_FETCH_GTEST=OFF`,
`RETHINKDB_SYMBOLS=ON`, `RETHINKDB_SPLIT_SYMBOLS=OFF`,
`RETHINKDB_STRIP_ON_INSTALL=OFF`, `RETHINKDB_BUILD_PORTABLE=ON`,
`RETHINKDB_ALLOCATOR=jemalloc`, and fetches Boost, RE2, QuickJS, jemalloc, and
Protobuf. The install prefix is `/usr`, `CMAKE_INSTALL_SYSCONFDIR` is `/etc`,
and `RETHINKDB_LOCALSTATEDIR` is `/var`.

`package-osx` sets `BUILD_TESTING=OFF`, leaves symbols in the binary, fetches
Boost, RE2, QuickJS, and Protobuf, and uses the prefix `/usr/local`.

List the presets visible on this machine:

```bash
cmake --list-presets
cmake --list-presets=build
```

## Options

Pass a cache option on the configure line, for example
`-DRETHINKDB_FETCH_PROTOBUF=ON`. Changing an option for an existing build
directory requires configuring that directory again.

### Dependencies

| Option | Default | Behavior |
| --- | --- | --- |
| `BUILD_TESTING` | `ON` | Build `rethinkdb-unittest` and register it with CTest |
| `RETHINKDB_FETCH_QUICKJS` | `ON` | Build the pinned QuickJS fork. `OFF` is a configure error |
| `RETHINKDB_FETCH_RE2` | `ON` | Build RE2 2015-11-01. `OFF` uses a system `re2` library and header |
| `RETHINKDB_FETCH_BOOST` | `ON` | Use the pinned Boost 1.84.0 headers. `OFF` requires Boost 1.84 or newer |
| `RETHINKDB_FETCH_GTEST` | `ON` | Build Google Test 1.8.1. Used when `BUILD_TESTING` is `ON` |
| `RETHINKDB_FETCH_JEMALLOC` | `ON` on Linux, otherwise `OFF` | Build jemalloc 5.2.1. Otherwise link a system `jemalloc` when that allocator is selected |
| `RETHINKDB_FETCH_PROTOBUF` | `ON` on Windows, otherwise `OFF` | Build Protobuf 36.2. Otherwise `find_package(Protobuf)` must succeed |
| `RETHINKDB_FETCH_OPENSSL` | `ON` on Windows, otherwise `OFF` | Build OpenSSL 3.0.7. Otherwise `find_package(OpenSSL)` must succeed |
| `RETHINKDB_FETCH_ZLIB` | `ON` on Windows, otherwise `OFF` | Build zlib 1.2.11. Otherwise `find_package(ZLIB)` must succeed |
| `RETHINKDB_FETCH_CURL` | `ON` on Windows, otherwise `OFF` | Build libcurl 7.82.0. Otherwise `find_package(CURL)` must succeed |
| `RETHINKDB_ALLOCATOR` | `jemalloc` on Linux, otherwise `system` | `jemalloc`, `tcmalloc`, or `system`. Any other value is a configure error |

`tcmalloc` links a system library named `tcmalloc` or `tcmalloc_minimal`.
When `RETHINKDB_FORCE_NATIVE` is `ON`, `RETHINKDB_COPY_NATIVE` and
`RETHINKDB_REDUCE_NATIVE` are not applied.

### Linking and symbols

| Option | Default | Behavior |
| --- | --- | --- |
| `RETHINKDB_ALLOW_WARNINGS` | `ON` | `OFF` adds `-Werror` |
| `RETHINKDB_SYMBOLS` | `ON` | Add `-g` to Release and MinSizeRel. On Linux, also link `-rdynamic` and `--build-id=sha1` |
| `RETHINKDB_SPLIT_SYMBOLS` | `OFF` | Strip the server and write `rethinkdb.debug`, or a `.dSYM` on macOS. Requires `RETHINKDB_SYMBOLS=ON` |
| `RETHINKDB_STRIP_ON_INSTALL` | `OFF` | Strip the binary at install time |
| `RETHINKDB_STATIC` | `OFF` | On Linux, link with `-static`, `-Wl,-Bdynamic`, `-static-libgcc`, and `-static-libstdc++` |
| `RETHINKDB_BUILD_PORTABLE` | `OFF` | On Linux, link `libgcc` statically. The `dist` target then also requires fetched Protobuf |
| `RETHINKDB_FORCE_NATIVE` | `OFF` | Pass `-march=native` |
| `RETHINKDB_COPY_NATIVE` | `OFF` | Pass `-march=<host processor>` |
| `RETHINKDB_REDUCE_NATIVE` | `OFF` | Pass a reduced `-march`. i586 and i686 become i486 |
| `RETHINKDB_NO_OMIT_FRAME_POINTER` | `OFF` | Keep frame pointers in non-debug builds |
| `RETHINKDB_KEEP_INLINE` | `OFF` | Pass `-fkeep-inline-functions` in Debug builds |
| `RETHINKDB_COVERAGE` | `OFF` | Pass `--coverage`. Configure fails unless the compiler is GCC |

`RETHINKDB_SPLIT_SYMBOLS` and `RETHINKDB_STRIP_ON_INSTALL` remove the symbols
CPack uses for the Debian `.ddeb` and the RPM debuginfo package. Leave both
`OFF` for those debug packages.

### Feature switches

Each option defaults to `OFF` and defines the preprocessor symbol in the last
column.

| Option | Preprocessor symbol |
| --- | --- |
| `RETHINKDB_LEGACY_GCC` | none. On GCC, suppress `-Wswitch-default` and `-Wswitch-enum` instead of `-Warray-bounds` and `-Wmaybe-uninitialized` |
| `RETHINKDB_LEGACY_LINUX` | `LEGACY_LINUX` and `NO_EPOLL` |
| `RETHINKDB_NO_EPOLL` | `NO_EPOLL` |
| `RETHINKDB_NO_EVENTFD` | `NO_EVENTFD` |
| `RETHINKDB_THREADED_COROUTINES` | `THREADED_COROUTINES` |
| `RETHINKDB_VALGRIND` | `VALGRIND` |
| `RETHINKDB_FULL_PERFMON` | `FULL_PERFMON` |
| `RETHINKDB_CORO_PROFILING` | `ENABLE_CORO_PROFILER` |
| `RETHINKDB_RQL_ERROR_BT` | `RQL_ERROR_BT` |
| `RETHINKDB_JSON_SHORTCUTS` | `JSON_SHORTCUTS` |
| `RETHINKDB_DISABLE_BREAKPOINTS` | `DISABLE_BREAKPOINTS` |

`RETHINKDB_VALGRIND=ON` requires `RETHINKDB_ALLOCATOR=system`.
`RETHINKDB_LEGACY_LINUX` also passes `-Wno-format`.

### Version, install paths, and Windows

| Option | Default | Behavior |
| --- | --- | --- |
| `RETHINKDB_VERSION` | empty | Packaging version. Empty uses `scripts/gen-version.sh` |
| `RETHINKDB_CODE_VERSION` | empty | Version written into the server and `VERSION.OVERRIDE`. Empty uses the script, or `RETHINKDB_VERSION` when that override differs from the script |
| `RETHINKDB_LOCALSTATEDIR` | `/var` when the prefix is `/usr`, otherwise `<prefix>/var` | Variable state directory |
| `RETHINKDB_NAMEVERSIONED` | `OFF` | Install the executable as `rethinkdb-<major>.<minor>` |
| `RETHINKDB_WEB_ASSETS_DIR` | empty | Admin UI directory read by the `web-assets` target |
| `RETHINKDB_WINDOWS_TOOLSET` | `v141` | MSVC toolset for fetched Windows dependencies |
| `RETHINKDB_WINDOWS_SDK` | `10.0.19041.0` | Windows SDK for fetched Windows dependencies |

## Targets

| Target | Where it is defined | What it produces |
| --- | --- | --- |
| `rethinkdb` | every host | The server binary |
| `rethinkdb-unittest` | `BUILD_TESTING=ON` | The unit-test binary. CTest name `rethinkdb-unittest` |
| `rethinkdb_man` | every host, part of `all` | `assets/rethinkdb.1.gz` in the build directory |
| `ql2_proto` | every host | Generated Query Language protocol sources |
| `install` | every host | Installed server, man page, license, init script, and config directories |
| `package` | every host | CPack TGZ of the install tree in `<binary-dir>/packages/tgz` |
| `dist` | every host | Source archive `<binary-dir>/packages/rethinkdb-<version>.tgz` |
| `package-deb` | Linux | Debian binary package in `<binary-dir>/packages/deb` |
| `package-rpm` | Linux | RPM in `<binary-dir>/packages/rpm` |
| `package-deb-src` | Linux | Debian source package prepared from `dist` |
| `package-osx` | macOS | `packages/osx/rethinkdb.dmg` |
| `generate-headers` | every host | Rewrites `src/rpc/serialize_macros.hpp` and `src/rpc/semilattice/joins/macros.hpp` in the source tree |
| `web-assets` | `RETHINKDB_WEB_ASSETS_DIR` is a directory | Rewrites `src/gen/web_assets.cc` from that admin UI tree |

`package-deb` and `package-rpm` depend on `rethinkdb` and `rethinkdb_man`.
`package-osx` does too. `package-deb-src` depends on `dist`, and `dist` depends
on `ql2_proto`. A normal build uses the committed `src/gen/web_assets.cc` and
does not require `web-assets`.

Build one target by name:

```bash
cmake --build build/release --target dist --parallel "$(sh cmake/parallel-jobs.sh)"
```

On a multi-configuration build, add `--config Release` or `--config Debug`.

## Check the CMake files

`scripts/check_cmake.sh` checks `CMakeLists.txt` and the files under `cmake/`
with [cmakelint](https://github.com/cmake-lint/cmake-lint) 1.4.3. Settings are
in `.cmakelintrc`. Install the checker with:

```bash
python -m pip install 'cmakelint==1.4.3'
```

CI runs that script in the preflight job, before the build.
