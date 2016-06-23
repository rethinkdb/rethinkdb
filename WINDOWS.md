# Building the Windows version of RethinkDB

## Requirements

* Visual Studio Community 2015 Update 1 (https://www.visualstudio.com/)

* CMake ("Windows Installer" from https://cmake.org/download/)

* The `precompiled/` directory from a RethinkDB source tarball. It can
  be built on Linux by running `make dist`.

* Cygwin (https://cygwin.com/install.html)

* Additional Cygwin packages: `setup.exe -q -P make curl patch git`

* If git is not configured yet,
  `git config --global user.name "John Doe"` and
  `git config --global user.email johndoe@example.com`

## Build Instructions

From a Cygwin shell:

```
./configure
make -j
```

Will first download and build the libraries that RethinkDB needs, then
build RethinkDB itself and place it in `build\Release_x64\rethinkdb.exe`