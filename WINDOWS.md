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

## RethinkDB

* Open `RethinkDB.sln`
* Make sure you are in the Debug/x64 configuration
* Press F7 to build
* If you've done everything right, the build will fail.