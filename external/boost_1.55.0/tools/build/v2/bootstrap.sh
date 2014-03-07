#!/bin/sh
# Copyright (C) 2005, 2006 Douglas Gregor.
# Copyright (C) 2006 The Trustees of Indiana University
# Copyright (C) 2010 Bryce Lelbach 
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# boostinspect:notab - Tabs are required for the Makefile.

B2=""
TOOLSET=""
B2_CONFIG=""

for option
do
    case $option in

    -help | --help | -h)
      want_help=yes ;;

    -with-toolset=* | --with-toolset=* )
      TOOLSET=`expr "x$option" : "x-*with-toolset=\(.*\)"`
      ;;

    -*)
      { echo "error: unrecognized option: $option
Try \`$0 --help' for more information." >&2
      { (exit 1); exit 1; }; }
      ;; 

    esac
done

if test "x$want_help" = xyes; then
  cat <<EOF
\`./bootstrap.sh' creates minimal Boost.Build, which can install itself.

Usage: $0 [OPTION]... 

Defaults for the options are specified in brackets.

Configuration:
  -h, --help                display this help and exit
  --with-b2=B2              use existing Boost.Build executable (b2)
                            [automatically built]
  --with-toolset=TOOLSET    use specific Boost.Build toolset
                            [automatically detected]
EOF
fi
test -n "$want_help" && exit 0

# TBD: Determine where the script is located
my_dir="."

# Determine the toolset, if not already decided
if test "x$TOOLSET" = x; then
  guessed_toolset=`$my_dir/engine/build.sh --guess-toolset`
  case $guessed_toolset in
    acc | darwin | gcc | como | mipspro | pathscale | pgi | qcc | vacpp )
    TOOLSET=$guessed_toolset
    ;;
    
    intel-* )
    TOOLSET=intel
    ;;
    
    mingw )
    TOOLSET=gcc
    ;;
    
    clang* )
    TOOLSET=clang
    ;;

    sun* )
    TOOLSET=sun
    ;;
    
    * )
    # Not supported by Boost.Build
    ;;
  esac
fi

case $TOOLSET in 
  clang*)
  TOOLSET=clang
  ;;
esac


rm -f config.log

# Build b2
if test "x$B2" = x; then
  echo -n "Bootstrapping the build engine with toolset $TOOLSET... "
  pwd=`pwd`
  (cd "$my_dir/engine" && ./build.sh "$TOOLSET") > bootstrap.log 2>&1
  if [ $? -ne 0 ]; then
      echo
      echo "Failed to bootstrap the build engine" 
      echo "Consult 'bootstrap.log' for more details"
      exit 1
  fi
  cd "$pwd"
  arch=`cd $my_dir/engine && ./bootstrap/jam0 -d0 -f build.jam --toolset=$TOOLSET --toolset-root= --show-locate-target && cd ..`
  B2="$my_dir/engine/$arch/b2"
  echo "engine/$arch/b2"
  cp "$B2" .
  cp "$my_dir/engine/$arch/bjam" .
fi

cat << EOF

Bootstrapping is done. To build and install, run:

    ./b2 install --prefix=<DIR>

EOF
