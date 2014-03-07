# 
# 
# THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED
# OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.
# 
# Permission is hereby granted to use or copy this program
# for any purpose,  provided the above notices are retained on all copies.
# Permission to modify the code and to distribute modified code is granted,
# provided the above notices are retained, and a notice that the code was
# modified is included with the above copyright notice.
#
# Modified by: Grzegorz Jakacki <jakacki at acm dot org>

# GC_SET_VERSION
# sets and AC_DEFINEs GC_VERSION_MAJOR, GC_VERSION_MINOR and GC_ALPHA_VERSION
# based on the contents of PACKAGE_VERSION; PACKAGE_VERSION must conform to
# [0-9]+[.][0-9]+(alpha[0.9]+)? 
# in lex syntax; if there is no alpha number, GC_ALPHA_VERSION is empty
#
AC_DEFUN(GC_SET_VERSION, [
  AC_MSG_CHECKING(GC version numbers)
  GC_VERSION_MAJOR=`echo $PACKAGE_VERSION | sed 's/^\([[0-9]][[0-9]]*\)[[.]].*$/\1/g'`
  GC_VERSION_MINOR=`echo $PACKAGE_VERSION | sed 's/^[[^.]]*[[.]]\([[0-9]][[0-9]]*\).*$/\1/g'`
  GC_ALPHA_VERSION=`echo $PACKAGE_VERSION | sed 's/^[[^.]]*[[.]][[0-9]]*//'`

  case "$GC_ALPHA_VERSION" in
    alpha*) 
      GC_ALPHA_VERSION=`echo $GC_ALPHA_VERSION \
      | sed 's/alpha\([[0-9]][[0-9]]*\)/\1/'` ;;
    *)  GC_ALPHA_MAJOR='' ;;
  esac

  if test :$GC_VERSION_MAJOR: = :: \
     -o   :$GC_VERSION_MINOR: = :: ;
  then
    AC_MSG_RESULT(invalid)
    AC_MSG_ERROR([nonconforming PACKAGE_VERSION='$PACKAGE_VERSION'])
  fi
  
  AC_DEFINE_UNQUOTED(GC_VERSION_MAJOR, $GC_VERSION_MAJOR)
  AC_DEFINE_UNQUOTED(GC_VERSION_MINOR, $GC_VERSION_MINOR)
  if test :$GC_ALPHA_VERSION: != :: ; then
    AC_DEFINE_UNQUOTED(GC_ALPHA_VERSION, $GC_ALPHA_VERSION)
  fi
  AC_MSG_RESULT(major=$GC_VERSION_MAJOR minor=$GC_VERSION_MINOR \
${GC_ALPHA_VERSION:+alpha=}$GC_ALPHA_VERSION)
])

sinclude(libtool.m4)
