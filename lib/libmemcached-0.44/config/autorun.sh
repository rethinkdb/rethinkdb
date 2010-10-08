#!/bin/sh
#
#  Copyright (c) 2006 Jan Kneschke
#  Copyright (c) 2009 Sun Microsystems
#  All rights reserved.
# 
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
# 
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. The name of the author may not be used to endorse or promote products
#     derived from this software without specific prior written permission.
# 
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Run this to generate all the initial makefiles, etc.

die() { echo "$@"; exit 1; }

# --force means overwrite ltmain.sh script if it already exists 
LIBTOOLIZE_FLAGS=" --automake --copy --force"
# --add-missing instructs automake to install missing auxiliary files
# and --force to overwrite them if they already exist
AUTOMAKE_FLAGS="--add-missing --copy --force --foreign"
ACLOCAL_FLAGS="-I m4"

ARGV0=$0
ARGS="$@"

run() {
	echo "$ARGV0: running \`$@' $ARGS"
	$@ $ARGS
}

# Try to locate a program by using which, and verify that the file is an
# executable
locate_binary() {
  for f in $@
  do
    file=`which $f 2>/dev/null | grep -v '^no '`
    if test -n "$file" -a -x "$file"; then
      echo $file
      return 0
    fi
  done

  echo "" 
  return 1
}


if test -f config/pre_hook.sh
then
  . config/pre_hook.sh
fi

# Try to detect the supported binaries if the user didn't
# override that by pushing the environment variable
if test x$LIBTOOLIZE = x; then
  LIBTOOLIZE=`locate_binary glibtoolize libtoolize-1.5 libtoolize`
  if test x$LIBTOOLIZE = x; then
    die "Did not find a supported libtoolize"
  fi
fi

if test x$ACLOCAL = x; then
  ACLOCAL=`locate_binary aclocal-1.11 aclocal-1.10 aclocal-1.9 aclocal19 aclocal`
  if test x$ACLOCAL = x; then
    die "Did not find a supported aclocal"
  fi
fi

if test x$AUTOMAKE = x; then
  AUTOMAKE=`locate_binary automake-1.11 automake-1.10 automake-1.9 automake19 automake`
  if test x$AUTOMAKE = x; then
    die "Did not find a supported automake"
  fi
fi

if test x$AUTOCONF = x; then
  AUTOCONF=`locate_binary autoconf-2.59 autoconf259 autoconf`
  if test x$AUTOCONF = x; then
    die "Did not find a supported autoconf"
  fi
fi

if test x$AUTOHEADER = x; then
  AUTOHEADER=`locate_binary autoheader-2.59 autoheader259 autoheader`
  if test x$AUTOHEADER = x; then
    die "Did not find a supported autoheader"
  fi
fi

run $LIBTOOLIZE $LIBTOOLIZE_FLAGS || die "Can't execute libtoolize"
run $ACLOCAL $ACLOCAL_FLAGS || die "Can't execute aclocal"
run $AUTOHEADER || die "Can't execute autoheader"
run $AUTOMAKE $AUTOMAKE_FLAGS  || die "Can't execute automake"
run $AUTOCONF || die "Can't execute autoconf"

if test -f config/post_hook.sh
then
  . config/post_hook.sh
fi

echo "---"
echo "Configured with the following tools:"
echo "  * `$LIBTOOLIZE --version | head -1`"
echo "  * `$ACLOCAL --version | head -1`"
echo "  * `$AUTOHEADER --version | head -1`"
echo "  * `$AUTOMAKE --version | head -1`"
echo "  * `$AUTOCONF --version | head -1`"
echo "---"
