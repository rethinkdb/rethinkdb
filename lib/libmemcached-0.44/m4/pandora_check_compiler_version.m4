dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.


AC_DEFUN([PANDORA_CHECK_C_VERSION],[

  dnl Print version of C compiler
  AC_MSG_CHECKING("C Compiler version--$GCC")
  AS_IF([test "$GCC" = "yes"],[
    CC_VERSION=`$CC --version | sed 1q`
  ],[AS_IF([test "$SUNCC" = "yes"],[
      CC_VERSION=`$CC -V 2>&1 | sed 1q`
    ],[
      CC_VERSION=""
    ])
  ])
  AC_MSG_RESULT("$CC_VERSION")
  AC_SUBST(CC_VERSION)
])


AC_DEFUN([PANDORA_CHECK_CXX_VERSION], [
  dnl Print version of CXX compiler
  AC_MSG_CHECKING("C++ Compiler version")
  AS_IF([test "$GCC" = "yes"],[
    CXX_VERSION=`$CXX --version | sed 1q`
  ],[AS_IF([test "$SUNCC" = "yes"],[
      CXX_VERSION=`$CXX -V 2>&1 | sed 1q`
    ],[
    CXX_VERSION=""
    ])
  ])
  AC_MSG_RESULT("$CXX_VERSION")
  AC_SUBST(CXX_VERSION)
])
