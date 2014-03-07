#!/bin/sh
# Copyright (C) 2005, 2006 Douglas Gregor.
# Copyright (C) 2006 The Trustees of Indiana University
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

# boostinspect:notab - Tabs are required for the Makefile.

BJAM=""
TOOLSET=""
BJAM_CONFIG=""
BUILD=""
PREFIX=/usr/local
EPREFIX=
LIBDIR=
INCLUDEDIR=
LIBS=""
PYTHON=python
PYTHON_VERSION=
PYTHON_ROOT=
ICU_ROOT=

# Internal flags
flag_no_python=
flag_icu=
flag_show_libraries=

for option
do
    case $option in

    -help | --help | -h)
      want_help=yes ;;

    -prefix=* | --prefix=*)
      PREFIX=`expr "x$option" : "x-*prefix=\(.*\)"`
      ;;

    -exec-prefix=* | --exec-prefix=*)
      EPREFIX=`expr "x$option" : "x-*exec-prefix=\(.*\)"`
      ;;

    -libdir=* | --libdir=*)
      LIBDIR=`expr "x$option" : "x-*libdir=\(.*\)"`
      ;;

    -includedir=* | --includedir=*)
      INCLUDEDIR=`expr "x$option" : "x-*includedir=\(.*\)"`
      ;;

    -show-libraries | --show-libraries )
      flag_show_libraries=yes
      ;;

    -with-bjam=* | --with-bjam=* )
      BJAM=`expr "x$option" : "x-*with-bjam=\(.*\)"`
      ;;

    -with-icu | --with-icu )
      flag_icu=yes
      ;;

    -with-icu=* | --with-icu=* )
      flag_icu=yes
      ICU_ROOT=`expr "x$option" : "x-*with-icu=\(.*\)"`
      ;;

    -without-icu | --without-icu )
      flag_icu=no
      ;;

    -with-libraries=* | --with-libraries=* )
      library_list=`expr "x$option" : "x-*with-libraries=\(.*\)"`
      if test "$library_list" != "all"; then
          old_IFS=$IFS
          IFS=,
          for library in $library_list
          do
              LIBS="$LIBS --with-$library"

              if test $library = python; then
                  requested_python=yes
              fi
          done
          IFS=$old_IFS

          if test "x$requested_python" != xyes; then
              flag_no_python=yes
          fi
      fi
      ;;

    -without-libraries=* | --without-libraries=* )
      library_list=`expr "x$option" : "x-*without-libraries=\(.*\)"`
      old_IFS=$IFS
      IFS=,
      for library in $library_list
      do
          LIBS="$LIBS --without-$library"

          if test $library = python; then
              flag_no_python=yes
          fi
      done
      IFS=$old_IFS
      ;;

    -with-python=* | --with-python=* )
      PYTHON=`expr "x$option" : "x-*with-python=\(.*\)"`
      ;;

    -with-python-root=* | --with-python-root=* )
      PYTHON_ROOT=`expr "x$option" : "x-*with-python-root=\(.*\)"`
      ;;

    -with-python-version=* | --with-python-version=* )
      PYTHON_VERSION=`expr "x$option" : "x-*with-python-version=\(.*\)"`
      ;;

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
\`./bootstrap.sh' prepares Boost for building on a few kinds of systems.

Usage: $0 [OPTION]... 

Defaults for the options are specified in brackets.

Configuration:
  -h, --help                display this help and exit
  --with-bjam=BJAM          use existing Boost.Jam executable (bjam)
                            [automatically built]
  --with-toolset=TOOLSET    use specific Boost.Build toolset
                            [automatically detected]
  --show-libraries          show the set of libraries that require build
                            and installation steps (i.e., those libraries
                            that can be used with --with-libraries or
                            --without-libraries), then exit
  --with-libraries=list     build only a particular set of libraries,
                            describing using either a comma-separated list of
                            library names or "all"
                            [all]
  --without-libraries=list  build all libraries except the ones listed []
  --with-icu                enable Unicode/ICU support in Regex 
                            [automatically detected]
  --without-icu             disable Unicode/ICU support in Regex
  --with-icu=DIR            specify the root of the ICU library installation
                            and enable Unicode/ICU support in Regex
                            [automatically detected]
  --with-python=PYTHON      specify the Python executable [python]
  --with-python-root=DIR    specify the root of the Python installation
                            [automatically detected]
  --with-python-version=X.Y specify the Python version as X.Y
                            [automatically detected]

Installation directories:
  --prefix=PREFIX           install Boost into the given PREFIX
                            [/usr/local]
  --exec-prefix=EPREFIX     install Boost binaries into the given EPREFIX
                            [PREFIX]

More precise control over installation directories:
  --libdir=DIR              install libraries here [EPREFIX/lib]
  --includedir=DIR          install headers here [PREFIX/include]

EOF
fi
test -n "$want_help" && exit 0

# TBD: Determine where the script is located
my_dir="."

# Determine the toolset, if not already decided
if test "x$TOOLSET" = x; then
  guessed_toolset=`$my_dir/tools/build/v2/engine/build.sh --guess-toolset`
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
    
    sun* )
    TOOLSET=sun
    ;;
    
    * )
    # Not supported by Boost.Build
    ;;
  esac
fi

rm -f config.log

# Build bjam
if test "x$BJAM" = x; then
  echo -n "Building Boost.Build engine with toolset $TOOLSET... "
  pwd=`pwd`
  (cd "$my_dir/tools/build/v2/engine" && ./build.sh "$TOOLSET") > bootstrap.log 2>&1
  if [ $? -ne 0 ]; then
      echo
      echo "Failed to build Boost.Build build engine" 
      echo "Consult 'bootstrap.log' for more details"
      exit 1
  fi
  cd "$pwd"
  arch=`cd $my_dir/tools/build/v2/engine && ./bootstrap/jam0 -d0 -f build.jam --toolset=$TOOLSET --toolset-root= --show-locate-target && cd ..`
  BJAM="$my_dir/tools/build/v2/engine/$arch/b2"
  echo "tools/build/v2/engine/$arch/b2"
  cp "$BJAM" .
  cp "$my_dir/tools/build/v2/engine/$arch/bjam" .

fi

# TBD: Turn BJAM into an absolute path

# If there is a list of libraries 
if test "x$flag_show_libraries" = xyes; then
  cat <<EOF

The following Boost libraries have portions that require a separate build
and installation step. Any library not listed here can be used by including
the headers only.

The Boost libraries requiring separate building and installation are:
EOF
  $BJAM -d0 --show-libraries | grep '^[[:space:]]*-'
  exit 0
fi

# Setup paths
if test "x$EPREFIX" = x; then
  EPREFIX="$PREFIX"
fi

if test "x$LIBDIR" = x; then
  LIBDIR="$EPREFIX/lib"
fi

if test "x$INCLUDEDIR" = x; then
  INCLUDEDIR="$PREFIX/include"
fi

# Find Python
if test "x$flag_no_python" = x; then
  result=`$PYTHON -c "exit" > /dev/null 2>&1`
  if [ "$?" -ne "0" ]; then
    flag_no_python=yes
  fi
fi

if test "x$flag_no_python" = x; then
    if test "x$PYTHON_VERSION" = x; then
        echo -n "Detecting Python version... "
        PYTHON_VERSION=`$PYTHON -c "import sys; print (\"%d.%d\" % (sys.version_info[0], sys.version_info[1]))"`
        echo $PYTHON_VERSION
    fi

    if test "x$PYTHON_ROOT" = x; then
        echo -n "Detecting Python root... "
        PYTHON_ROOT=`$PYTHON -c "import sys; print sys.prefix"`
        echo $PYTHON_ROOT
    fi    
fi

# Configure ICU
echo -n "Unicode/ICU support for Boost.Regex?... "
if test "x$flag_icu" != xno; then
  if test "x$ICU_ROOT" = x; then
    COMMON_ICU_PATHS="/usr /usr/local /sw"
    for p in $COMMON_ICU_PATHS; do
      if test -r $p/include/unicode/utypes.h; then
        ICU_ROOT=$p
      fi
    done
  
    if test "x$ICU_ROOT" = x; then
      echo "not found."
    else      
      BJAM_CONFIG="$BJAM_CONFIG -sICU_PATH=$ICU_ROOT"
      echo "$ICU_ROOT"
    fi
  else
    BJAM_CONFIG="$BJAM_CONFIG -sICU_PATH=$ICU_ROOT"
    echo "$ICU_ROOT"
  fi
else
  echo "disabled."
fi

# Backup the user's existing project-config.jam
JAM_CONFIG_OUT="project-config.jam"
if test -r "project-config.jam"; then
  counter=1
 
  while test -r "project-config.jam.$counter"; do
    counter=`expr $counter + 1`
  done

  echo "Backing up existing Boost.Build configuration in project-config.jam.$counter"
  mv "project-config.jam" "project-config.jam.$counter"
fi

# Generate user-config.jam
echo "Generating Boost.Build configuration in project-config.jam..."
cat > project-config.jam <<EOF
# Boost.Build Configuration
# Automatically generated by bootstrap.sh

import option ;
import feature ;

# Compiler configuration. This definition will be used unless
# you already have defined some toolsets in your user-config.jam
# file.
if ! $TOOLSET in [ feature.values <toolset> ]
{
    using $TOOLSET ; 
}

project : default-build <toolset>$TOOLSET ;
EOF

#  - Python configuration
if test "x$flag_no_python" = x; then
  cat >> project-config.jam <<EOF

# Python configuration
using python : $PYTHON_VERSION : $PYTHON_ROOT ;
EOF
fi

if test "x$ICU_ROOT" != x; then
  cat >> project-config.jam << EOF

path-constant ICU_PATH : $ICU_ROOT ;

EOF
fi

cat >> project-config.jam << EOF

# List of --with-<library> and --without-<library>
# options. If left empty, all libraries will be built.
# Options specified on the command line completely
# override this variable.
libraries = $LIBS ;

# These settings are equivivalent to corresponding command-line
# options.
option.set prefix : $PREFIX ;
option.set exec-prefix : $EPREFIX ;
option.set libdir : $LIBDIR ;
option.set includedir : $INCLUDEDIR ;

# Stop on first error
option.set keep-going : false ;
EOF

cat << EOF

Bootstrapping is done. To build, run:

    ./b2
    
To adjust configuration, edit 'project-config.jam'.
Further information:

   - Command line help:
     ./b2 --help
     
   - Getting started guide: 
     http://www.boost.org/more/getting_started/unix-variants.html
     
   - Boost.Build documentation:
     http://www.boost.org/boost-build2/doc/html/index.html

EOF