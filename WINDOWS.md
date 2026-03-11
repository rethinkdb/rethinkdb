# Building the Windows version of RethinkDB

## Requirements

### Visual Studio

Supported versions: Visual Studio 2017, 2019, or 2022

> **Note**
> VS must be installed in default directory (any drive)

* In installer tick desktop C++ workload, and in individual components:
* MSVC v141 (for VS2017/2019/2022 compatibility)
* Windows 10 SDK 10.0.19041.0
* CMake tool

#### Environment Variables (Optional)

To customize the Visual Studio build environment, you can set these variables:

* `VS_GENERATOR` - CMake generator (default: "Visual Studio 17 2022")
  * For VS2017: `export VS_GENERATOR="Visual Studio 15 2017"`
  * For VS2019: `export VS_GENERATOR="Visual Studio 16 2019"`
* `VCTOOLS_VERSION` - Platform toolset (default: "v141")
  * Use "v141" for VS2017 compatibility
  * Use "v142" for VS2019 only builds
  * Use "v143" for VS2022 only builds
* `VCVARS_VER` - VC++ toolset version (default: "14.1")
* `WINDOWSSDKVERSION` - Windows SDK version (default: "10.0.19041.0")

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
