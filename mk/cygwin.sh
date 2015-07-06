#!/bin/bash

set -eu 
set -o pipefail

MSBUILD=/cygdrive/c/Program\ Files\ \(x86\)/MSBuild/14.0/Bin/MSBuild.exe

convertpaths () {
    # perl -pe 's/([A-Za-z]:\\([^\\]+\\)*([^ :\n"'"'"'])*)/`cygpath "$1"`/g'
    perl -pe 's|([a-zA-Z]):\\|/cygdrive/\1/|g; s|\\|/|g'
}

"$MSBUILD" /maxcpucount "$@" | convertpaths
