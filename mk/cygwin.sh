#!/bin/bash

set -eu
set -o pipefail

MSBUILD=/cygdrive/c/Program\ Files\ \(x86\)/MSBuild/14.0/Bin/MSBuild.exe

convertpaths () {
    # perl -pe 's/([A-Za-z]:\\([^\\]+\\)*([^ :\n"'"'"'])*)/`cygpath "$1"`/g'
    perl -ne '
      s|([a-zA-Z]):\\|/cygdrive/\1/|g;          # convert to cygwin path
      s|\\|/|g;                                 # mirror slashes
      print unless m{^         x64/.*\.obj.?$}' # don't list all object files when linking
}

"$MSBUILD" /maxcpucount "$@" | convertpaths
