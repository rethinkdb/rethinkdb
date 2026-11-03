# Building the Windows version of RethinkDB

The Windows build uses CMake with the Visual Studio generator, toolset v141,
and the Windows 10 SDK 10.0.19041.0. The server links the static CRT
(`/MT` in Release, `/MTd` in Debug).

## Requirements

### Visual Studio 2022

Install the desktop C++ workload, plus:

- MSVC v141
- Windows 10 SDK 10.0.19041.0
- CMake

### Strawberry Perl

Required to build the fetched OpenSSL. Put it on `PATH`. Do not let a Cygwin
`perl` shadow it.

### Premake5

Required to build the fetched QuickJS. Put `premake5` on `PATH`.

### NMake

Use the Visual Studio developer command prompt so `nmake` and `msbuild` are
available to the fetched OpenSSL, zlib, and curl builds.

## Configure and build

From a Visual Studio developer prompt:

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -T v141 -A x64 ^
  -DRETHINKDB_FETCH_BOOST=ON ^
  -DRETHINKDB_FETCH_RE2=ON ^
  -DRETHINKDB_FETCH_QUICKJS=ON ^
  -DRETHINKDB_FETCH_GTEST=ON ^
  -DRETHINKDB_FETCH_PROTOBUF=ON ^
  -DRETHINKDB_FETCH_OPENSSL=ON ^
  -DRETHINKDB_FETCH_ZLIB=ON ^
  -DRETHINKDB_FETCH_CURL=ON

cmake --build build --config Release --parallel
```

Those fetch options default to `ON` when CMake detects Windows, so a plain
`cmake -S . -B build -G "Visual Studio 17 2022" -T v141 -A x64` selects the
same pins. The allocator stays the system allocator. The unit-test binary is
built when `BUILD_TESTING` is `ON` (the default).

Output:

- `build/Release/rethinkdb.exe`
- `build/Debug/rethinkdb-unittest.exe` when the Debug configuration is built

`src/arch/windows_stub` is on the include path so the stub POSIX headers are
used. It is not on the include path of Unix builds.
