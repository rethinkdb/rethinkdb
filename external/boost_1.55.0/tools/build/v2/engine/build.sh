#!/bin/sh

#~ Copyright 2002-2005 Rene Rivera.
#~ Distributed under the Boost Software License, Version 1.0.
#~ (See accompanying file LICENSE_1_0.txt or copy at
#~ http://www.boost.org/LICENSE_1_0.txt)

# Reset the toolset.
BOOST_JAM_TOOLSET=

# Run a command, and echo before doing so. Also checks the exit status and quits
# if there was an error.
echo_run ()
{
    echo "$@"
    $@
    r=$?
    if test $r -ne 0 ; then
        exit $r
    fi
}

# Print an error message, and exit with a status of 1.
error_exit ()
{
    echo "###"
    echo "###" "$@"
    echo "###"
    echo "### You can specify the toolset as the argument, i.e.:"
    echo "###     ./build.sh gcc"
    echo "###"
    echo "### Toolsets supported by this script are:"
    echo "###     acc, como, darwin, gcc, intel-darwin, intel-linux, kcc, kylix,"
    echo "###     mipspro, mingw(msys), pathscale, pgi, qcc, sun, sunpro, tru64cxx, vacpp"
    echo "###"
    echo "### A special toolset; cc, is available which is used as a fallback"
    echo "### when a more specific toolset is not found and the cc command is"
    echo "### detected. The 'cc' toolset will use the CC, CFLAGS, and LIBS"
    echo "### environment variables, if present."
    echo "###"
    exit 1
}

# Check that a command is in the PATH.
test_path ()
{
    if `command -v command 1>/dev/null 2>/dev/null`; then
        command -v $1 1>/dev/null 2>/dev/null
    else
        hash $1 1>/dev/null 2>/dev/null
    fi
}

# Check that the OS name, as returned by "uname", is as given.
test_uname ()
{
    if test_path uname; then
        test `uname` = $*
    fi
}

# Try and guess the toolset to bootstrap the build with...
Guess_Toolset ()
{
    if test -r /mingw/bin/gcc ; then
        BOOST_JAM_TOOLSET=mingw
        BOOST_JAM_TOOLSET_ROOT=/mingw/
    elif test_uname Darwin ; then BOOST_JAM_TOOLSET=darwin
    elif test_uname IRIX ; then BOOST_JAM_TOOLSET=mipspro
    elif test_uname IRIX64 ; then BOOST_JAM_TOOLSET=mipspro
    elif test_uname OSF1 ; then BOOST_JAM_TOOLSET=tru64cxx
    elif test_uname QNX && test_path qcc ; then BOOST_JAM_TOOLSET=qcc
    elif test_path gcc ; then BOOST_JAM_TOOLSET=gcc
    elif test_path icc ; then BOOST_JAM_TOOLSET=intel-linux
    elif test -r /opt/intel/cc/9.0/bin/iccvars.sh ; then
        BOOST_JAM_TOOLSET=intel-linux
        BOOST_JAM_TOOLSET_ROOT=/opt/intel/cc/9.0
    elif test -r /opt/intel_cc_80/bin/iccvars.sh ; then
        BOOST_JAM_TOOLSET=intel-linux
        BOOST_JAM_TOOLSET_ROOT=/opt/intel_cc_80
    elif test -r /opt/intel/compiler70/ia32/bin/iccvars.sh ; then
        BOOST_JAM_TOOLSET=intel-linux
        BOOST_JAM_TOOLSET_ROOT=/opt/intel/compiler70/ia32/
    elif test -r /opt/intel/compiler60/ia32/bin/iccvars.sh ; then
        BOOST_JAM_TOOLSET=intel-linux
        BOOST_JAM_TOOLSET_ROOT=/opt/intel/compiler60/ia32/
    elif test -r /opt/intel/compiler50/ia32/bin/iccvars.sh ; then
        BOOST_JAM_TOOLSET=intel-linux
        BOOST_JAM_TOOLSET_ROOT=/opt/intel/compiler50/ia32/
    elif test_path pgcc ; then BOOST_JAM_TOOLSET=pgi
    elif test_path pathcc ; then BOOST_JAM_TOOLSET=pathscale
    elif test_path xlc ; then BOOST_JAM_TOOLSET=vacpp
    elif test_path como ; then BOOST_JAM_TOOLSET=como
    elif test_path KCC ; then BOOST_JAM_TOOLSET=kcc
    elif test_path bc++ ; then BOOST_JAM_TOOLSET=kylix
    elif test_path aCC ; then BOOST_JAM_TOOLSET=acc
    elif test_uname HP-UX ; then BOOST_JAM_TOOLSET=acc
    elif test -r /opt/SUNWspro/bin/cc ; then
        BOOST_JAM_TOOLSET=sunpro
        BOOST_JAM_TOOLSET_ROOT=/opt/SUNWspro/
    # Test for "cc" as the default fallback.
    elif test_path $CC ; then BOOST_JAM_TOOLSET=cc
    elif test_path cc ; then
        BOOST_JAM_TOOLSET=cc
        CC=cc
    fi
    if test "$BOOST_JAM_TOOLSET" = "" ; then
        error_exit "Could not find a suitable toolset."
    fi
}

# The one option we support in the invocation
# is the name of the toolset to force building
# with.
case "$1" in
    --guess-toolset) Guess_Toolset ; echo "$BOOST_JAM_TOOLSET" ; exit 1 ;;
    -*) Guess_Toolset ;;
    ?*) BOOST_JAM_TOOLSET=$1 ; shift ;;
    *) Guess_Toolset ;;
esac
BOOST_JAM_OPT_JAM="-o bootstrap/jam0"
BOOST_JAM_OPT_MKJAMBASE="-o bootstrap/mkjambase0"
BOOST_JAM_OPT_YYACC="-o bootstrap/yyacc0"
case $BOOST_JAM_TOOLSET in
    mingw)
    if test -r ${BOOST_JAM_TOOLSET_ROOT}bin/gcc ; then
        export PATH=${BOOST_JAM_TOOLSET_ROOT}bin:$PATH
    fi
    BOOST_JAM_CC="gcc -DNT"
    ;;

    gcc)
    BOOST_JAM_CC=gcc
    ;;

    darwin)
    BOOST_JAM_CC=cc
    ;;

    intel-darwin)
    BOOST_JAM_CC=icc
    ;;

    intel-linux)
    if test -r /opt/intel/cc/9.0/bin/iccvars.sh ; then
        BOOST_JAM_TOOLSET_ROOT=/opt/intel/cc/9.0/
    elif test -r /opt/intel_cc_80/bin/iccvars.sh ; then
        BOOST_JAM_TOOLSET_ROOT=/opt/intel_cc_80/
    elif test -r /opt/intel/compiler70/ia32/bin/iccvars.sh ; then
        BOOST_JAM_TOOLSET_ROOT=/opt/intel/compiler70/ia32/
    elif test -r /opt/intel/compiler60/ia32/bin/iccvars.sh ; then
        BOOST_JAM_TOOLSET_ROOT=/opt/intel/compiler60/ia32/
    elif test -r /opt/intel/compiler50/ia32/bin/iccvars.sh ; then
        BOOST_JAM_TOOLSET_ROOT=/opt/intel/compiler50/ia32/
    fi
    if test -r ${BOOST_JAM_TOOLSET_ROOT}bin/iccvars.sh ; then
        # iccvars does not change LD_RUN_PATH. We adjust LD_RUN_PATH here in
        # order not to have to rely on ld.so.conf knowing the icc library
        # directory. We do this before running iccvars.sh in order to allow a
        # user to add modifications to LD_RUN_PATH in iccvars.sh.
        if test -z "${LD_RUN_PATH}"; then
            LD_RUN_PATH="${BOOST_JAM_TOOLSET_ROOT}lib"
        else
            LD_RUN_PATH="${BOOST_JAM_TOOLSET_ROOT}lib:${LD_RUN_PATH}"
        fi
        export LD_RUN_PATH
        . ${BOOST_JAM_TOOLSET_ROOT}bin/iccvars.sh
    fi
    BOOST_JAM_CC=icc
    ;;

    vacpp)
    BOOST_JAM_CC=xlc
    ;;

    como)
    BOOST_JAM_CC="como --c"
    ;;

    kcc)
    BOOST_JAM_CC=KCC
    ;;

    kylix)
    BOOST_JAM_CC=bc++
    ;;

    mipspro)
    BOOST_JAM_CC=cc
    ;;

    pathscale)
    BOOST_JAM_CC=pathcc
    ;;

    pgi)
    BOOST_JAM_CC=pgcc
    ;;

    sun*)
    if test -z "${BOOST_JAM_TOOLSET_ROOT}" -a -r /opt/SUNWspro/bin/cc ; then
        BOOST_JAM_TOOLSET_ROOT=/opt/SUNWspro/
    fi
    if test -r "${BOOST_JAM_TOOLSET_ROOT}bin/cc" ; then
        PATH=${BOOST_JAM_TOOLSET_ROOT}bin:${PATH}
        export PATH
    fi
    BOOST_JAM_CC=cc
    ;;

    clang*)
    BOOST_JAM_CC="clang -Wno-unused -Wno-format"
    BOOST_JAM_TOOLSET=clang
    ;;

    tru64cxx)
    BOOST_JAM_CC=cc
    ;;

    acc)
    BOOST_JAM_CC="cc -Ae"
    ;;

    cc)
    if test -z "$CC" ; then CC=cc ; fi
    BOOST_JAM_CC=$CC
    BOOST_JAM_OPT_JAM="$BOOST_JAM_OPT_JAM $CFLAGS $LIBS"
    BOOST_JAM_OPT_MKJAMBASE="$BOOST_JAM_OPT_MKJAMBASE $CFLAGS $LIBS"
    BOOST_JAM_OPT_YYACC="$BOOST_JAM_OPT_YYACC $CFLAGS $LIBS"
    ;;

    qcc)
    BOOST_JAM_CC=qcc
    ;;

    *)
    error_exit "Unknown toolset: $BOOST_JAM_TOOLSET"
    ;;
esac

echo "###"
echo "### Using '$BOOST_JAM_TOOLSET' toolset."
echo "###"

YYACC_SOURCES="yyacc.c"
MKJAMBASE_SOURCES="mkjambase.c"
BJAM_SOURCES="\
 command.c compile.c constants.c debug.c execcmd.c frames.c function.c glob.c\
 hash.c hdrmacro.c headers.c jam.c jambase.c jamgram.c lists.c make.c make1.c\
 object.c option.c output.c parse.c pathsys.c regexp.c rules.c\
 scan.c search.c subst.c timestamp.c variable.c modules.c strings.c filesys.c\
 builtins.c class.c cwd.c native.c md5.c w32_getreg.c modules/set.c\
 modules/path.c modules/regex.c modules/property-set.c modules/sequence.c\
 modules/order.c"
case $BOOST_JAM_TOOLSET in
    mingw)
    BJAM_SOURCES="${BJAM_SOURCES} execnt.c filent.c pathnt.c"
    ;;

    *)
    BJAM_SOURCES="${BJAM_SOURCES} execunix.c fileunix.c pathunix.c"
    ;;
esac

BJAM_UPDATE=
if test "$1" = "--update" -o "$2" = "--update" -o "$3" = "--update" -o "$4" = "--update" ; then
    BJAM_UPDATE="update"
fi
if test "${BJAM_UPDATE}" = "update" -a ! -x "./bootstrap/jam0" ; then
    BJAM_UPDATE=
fi

if test "${BJAM_UPDATE}" != "update" ; then
    echo_run rm -rf bootstrap
    echo_run mkdir bootstrap
    if test ! -r jamgram.y -o ! -r jamgramtab.h ; then
        echo_run ${BOOST_JAM_CC} ${BOOST_JAM_OPT_YYACC} ${YYACC_SOURCES}
        if test -x "./bootstrap/yyacc0" ; then
            echo_run ./bootstrap/yyacc0 jamgram.y jamgramtab.h jamgram.yy
        fi
    fi
    if test ! -r jamgram.c -o ! -r jamgram.h ; then
        if test_path yacc ; then YACC="yacc -d"
        elif test_path bison ; then YACC="bison -y -d --yacc"
        fi
        echo_run $YACC jamgram.y
        mv -f y.tab.c jamgram.c
        mv -f y.tab.h jamgram.h
    fi
    if test ! -r jambase.c ; then
        echo_run ${BOOST_JAM_CC} ${BOOST_JAM_OPT_MKJAMBASE} ${MKJAMBASE_SOURCES}
        if test -x "./bootstrap/mkjambase0" ; then
            echo_run ./bootstrap/mkjambase0 jambase.c Jambase
        fi
    fi
    echo_run ${BOOST_JAM_CC} ${BOOST_JAM_OPT_JAM} ${BJAM_SOURCES}
fi
if test -x "./bootstrap/jam0" ; then
    if test "${BJAM_UPDATE}" != "update" ; then
        echo_run ./bootstrap/jam0 -f build.jam --toolset=$BOOST_JAM_TOOLSET "--toolset-root=$BOOST_JAM_TOOLSET_ROOT" "$@" clean
    fi
    echo_run ./bootstrap/jam0 -f build.jam --toolset=$BOOST_JAM_TOOLSET "--toolset-root=$BOOST_JAM_TOOLSET_ROOT" "$@"
fi
