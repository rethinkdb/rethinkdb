#!/bin/bash

# This script builds RethinkDB using VC++ from the cygwin command line

set -eu
set -o pipefail

MSBUILD=/cygdrive/c/Program\ Files\ \(x86\)/MSBuild/14.0/Bin/MSBuild.exe

format_output () {
    perl -ne '
      s|([a-zA-Z]):\\|/cygdrive/\1/|g;          # convert to cygwin path
      s|\\|/|g;                                 # mirror slashes
      s|( [^ ]+\.cc){10,}| ...|;                # do not list all cc files on a single line
      print unless m{^         x64/.*\.obj.?$}' # do not list all object files when linking
}

"$MSBUILD" /p:PlatformToolset=v140 /property:Platform=x64 /maxcpucount "$@" RethinkDB.vcxproj | format_output
