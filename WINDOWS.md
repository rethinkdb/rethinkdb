# Building the Windows version of RethinkDB

## Requirements

### [Visual Studio 2022](https://visualstudio.microsoft.com/vs)
> **Note**
> VS must be installed in default directory (any drive)

* In installer tick desktop C++ workload, and in individual components:
* MSVC v141
* Windows 10 SDK 10.0.19041.0
* CMake tool

### [Cygwin](https://cygwin.com/install.html)
> **Warning**
> Make sure that `perl` package IS NOT installed in cygwin!

Additional Cygwin packages:
* make
* curl
* wget
* patch

### [Strawberry Perl](https://strawberryperl.com/) (Required for OpenSSL)
Install and make sure it's added to PATH

### [Premake5](https://premake.github.io/download/) (Required for quickjspp)
Download, unpack anywhere and add to PATH

## Build Instructions

From a Cygwin shell:

```
./configure

make -j$(nproc)
```

Will first download and build the libraries that RethinkDB needs, then
build RethinkDB itself and place it in `build\Release_x64\rethinkdb.exe`

If `make` complains about missing files in `mk/gen`, run `mkdir mk/gen` manually.
Then run `make -j` again.
