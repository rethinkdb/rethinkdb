# Building the Windows version of RethinkDB

## Requirements

* Visual Studio Community 2015 Update 1 (https://www.visualstudio.com/)

* CMake ("Windows Installer" from https://cmake.org/download/)

* node.js with NPM (https://nodejs.org/en/download/)

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

If `make` complains about missing files in `mk/gen`, run `mkdir mk/gen` manually.
Then run `make -j` again.

On some systems, you might get permission errors like the ones described in
https://github.com/npm/npm/issues/10826 during the build from npm.
Try running `npm install -g JSONStream` and then run `make -j` again to work
around this issue.
