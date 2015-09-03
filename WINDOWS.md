# Notes for building on windows

I am in the process of porting RethinkDB to Windows. These notes may
or may not help you reproduce builds that I have managed to run so
far.

## Visual Studio

* Install Visual Studio 2015 (Community Edition)

## Cygwin

* `mk/cygwin.sh` is a wrapper around msbuild that converts error messages to ones
  that cygwin's emacs can understand

## Command line

I've been alternating between the cygwin command line and cmd.exe.
Some environment variables I've neeed are:

* PATH="/cygdrive/c/Program Files (x86)/Microsoft Visual Studio 14.0/VC/bin/amd64":$PATH
* PATH="/cygdrive/c/Program Files (x86)/Windows Kits/8.1/bin/x64/":$PATH
* export INCLUDE="C:/Program Files (x86)/Windows Kits/8.1/Include/um/;C:/Program Files (x86)/Windows Kits/8.1/Include/shared;c:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/include;c:/Program Files (x86)/Windows Kits/10/Include/10.0.10150.0/ucrt"
* export LIB="c:/Program Files (x86)/Windows Kits/8.1/Lib/winv6.3/um/x64/;c:/Program Files (x86)/Microsoft Visual Studio 14.0/
VC/lib/amd64;c:/Program Files (x86)/Windows Kits/10/Lib/10.0.10056.0/ucrt/x64"

## Dependencies

Follow the instructions below to build the dependencies.

Pre-built static libraries and headers are available in ipfs://QmYVvWsNPr5ixpJQz1atf6hfeTnYfy6VuEpPosYLpUzniA/windows_deps.zip

## boost

* download boost 1.58.0 into `..\boost_1_58_0`
* in the boost directory, run

```
.\bootstrap.bat
.\b2 -j 24 runtime-link=static address-model=64
```

* Copy the files from `../boost_1_58_0/bin.v2/libs/date_time/build/msvc-14.0/debug/address-model-64/link-static/runtime-link-static/threading-multi/`
  to `windows_deps/lib/Debug`
* Copy the `../boost_1_58_0/boost` folder to `windows_deps/include`

## gtest

* install cmake and python. add both to your PATH.
* in `external\gtest_1.7.0`, run

```
cmake -G"Visual Studio 14 Win64"
```

* Open `gtest.sln` in visual studio.
* Build the `gtest` project.
* copy `external/gtest_1.7.0/Debug/*` to `windows_deps/lib/Debug`
* copy `external/gtest_1.7.0/include/gtest` to `windows_deps/include`

## Protobuf

* Download protobuf into `..\protobuf-2.5.0`
* Open `vsprojects\protobuf.sln`
* Open Build -> Configuration Manager and add an x64 configuration based on Win32
* For each project in `libprotobuf` `libprotoc` and `protoc`:
  - Open Properties -> C/C++ -> All Options
  - Set Runtime Library to `/MTd`
* Copy `../protobuf-2.5.0/vsprojects/x64/Debug/libprotobuf.lib` to `windows_deps/lib/Debug`
* Copy the files from `../protobuf-2.5.0/vsprojects/include` to `windows_deps/include`

## OpenSSL

* Download `openssl-1.0.2a.tar.gz` from https://www.openssl.org/source/
* Extract it to `../openssl-1.0.2a`
* `cd ../openssl-1.0.2a`
* Extracting the tar file may have created symlinks that don't work in VC++. They can be converted by doing ```for x in `find . -type l`; do mv -f `readlink $x` $x; done```
* To build, maybe follow the instructions in `INSTALL.w64`
* Or instead, download from http://p-nand-q.com/programming/windows/building_openssl_with_visual_studio_2013.html
* Copy the `include/openssl` folder to `windows_deps/include`
* Copy `libeay32.lib` `ssleay32.lib` to `windows_deps/lib/Debug`

## ICU

* Use v8's icu
* Recursively copy all .h files from `../v8/third_party/icu/source/common` to `windows_deps/include`

## RE2

* ... TODO build
* Copy `../re2/x64/Debug/re2.lib` `windows_deps/lib/Debug`
* Copy `../re2/re2/*.h` to `windows_deps/include/re2`

## Curl

* `make fetch-curl`
* cd external/curl_7.40.0/winbuild
* nmake /f Makefile.vc mode=static DEBUG=yes MACHINE=x64 RTLIBCFG=static
* copy `external/curl_7.40.0/builds/libcurl-vc-x64-debug-static-ipv6-sspi-winssl/lib/libcurl_a_debug.lib`
  to `windows_deps/libs/Debug/curl.lib`
* Copy `external/curl_7.40.0/include/curl` to `windows_deps/include`

## V8

* Download v8 4.7.17 into `../v8` (see https://code.google.com/p/v8-wiki/wiki/UsingGit)
* Build it in Debug mode for x64 using VS2015 (see https://code.google.com/p/v8-wiki/wiki/BuildingWithGYP)
 * Pass `-Dtarget_arch=x64` to `gyp_v8`
* See also https://developers.google.com/v8/get_started
* Copy `../v8/include/*` to `windows_deps/include`
* In `windows_deps/include/libplatform/libplatform.h`, change `include/v8-platform.h` to `v8-platform.h`
* Copy `../v8/build/Debug/lib/{v8,icu}*.lib` to `windows_deps/lib/Debug/`

## ZLib

* `make fetch-zlib`
* `cd external/zlib_1.2.8`
* Edit `win32/Makefile.msc`, change `-MD` to `-MTd`
* `nmake -f win32/Makefile.msc`
* Copy `external/zlib_1.2.8/z{lib,conf}.h` to `windows_deps/include`

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
* Check all RSI ATN TODO _MSC_VER and _WIN32 tags
* Handle ^C
* test blocker pool, maybe use windowswindows-specific pool
* make sure it still builds on linux
* keep up-to-date with next/raft/sunos
* rebuild with MSC and examine all the int32 -> int64 warnings.
* restore the msvc-specific s2 code
* ensure that aligned pointers are freed and allocated correctly
* install as service
* ensure windows paths are parsed and generated correctly where needed
* memset 0 / bzero
