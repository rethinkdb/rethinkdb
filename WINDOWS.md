# Notes for building on windows

I am in the process of porting RethinkDB to Windows. These
notes may or may not help you reproduce the failing builds
that I have managed to run so far.

## Visual Studio

* Install Visual Studio 2015 RC

## boost

* downlaod boost 1.58.0 into `..\boost_1_58_0`
* in the boost directory, run

```
.\bootstrap.bat
.\b2 -j 24 runtime-link=static address-model=64
```

## gtest

* install cmake and python. add both to your PATH.
* in `external\gtest_1.7.0`, run

```
cmake -G"Visual Studio 14 Win64"
```

* Open `gtest.sln` in visual studio.
* Build the `gtest` project.

## Protobuf

The dependency on Protobuf will hopefully go away soon.

* Download protobuf into `..\protobuf-2.5.0`
* Open `vsprojects\protobuf.sln`
* Open Build -> Configuration Manager and add an x64 configuration based on Win32
* For each project in `libprotobuf` `libprotoc` and `protoc`:
  - Open Properties -> C/C++ -> All Options
  - Set Runtime Library to `/MTd`

## OpenSSL

* Download `openssl-1.0.2a.tar.gz` from https://www.openssl.org/source/
* Extract it to `../openssl-1.0.2a`
* Extracting the tar file may have created symlinks that don't work in VC++. They can be converted by doing ```for x in `find . -type l`; do mv -f `readlink $x` $x; done```
* TODO: build

## ICU

* cd to the RethinkDB folder from the cygwin shell
* Convert endings of sh files to unix-style. Use dos2unix or ```perl -pi -e chomp `find . -name \*.sh` ````
* `./configure --continue --fetch all`
* `make fetch-icu`
* ...

## RE2

* ...

## RethinkDB

* Open `RethinkDB.sln`
* Make sure you are in the Debug/x64 configuration
* Press F7 to build
* Press F5 to run
* Some unit tests should run.

# TODO

* Compile with and fix all warnings
* Support mingw64
* Clean up the ifdefs, and use feature macros
* Command line build using msbuild
* Consider not using the pthread API on windows
* Backtraces on crash, with addr2line
* Clean up the arch hierachy
* Check for misnamed linux_ prefixess
* Thouroughly test clock time and ticks
* Test the timers
* sizeof int and sizeof long differences
* Generate web assets
* Generate protobuf
* Pass all unit tests
* death tests with exceptions seem to fail
* web assets literal size > 65535
* Check all RSI ATN TODO _MSC_VER and _WIN32 tags
* re-implement sockets.cc
* Handle ^C
* no more named VA_ARGS
* test blocker pool, maybe use windowswindows-specific pool
* make sure it still builds on linux
* keep up-to-date with next/raft/sunos
* build without /force:unresolved LDFLAG 
* rebuild with MSC and examine all the int32 -> int64 warnings.
* restore the msvc-specific s2 code
* ensure that aligned pointers are freed correctly
* install as service
* ensure windows paths are parsed and generated correctly where needed