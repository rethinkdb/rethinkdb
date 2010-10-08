#
# Some platforms define EWOULDBLOCK == EAGAIN, causing our switch for error
# codes to be illegal (POSIX.1-2001 allows both return codes from recv, so
# we need to test both if they differ...)
#
AC_DEFUN([DETECT_EAGAIN],
[
    AC_CACHE_CHECK([if EWOULDBLOCK == EAGAIN],[av_cv_eagain_ewouldblock],
        [AC_TRY_COMPILE([
#include <errno.h>
                        ], [
int error = EAGAIN;
switch (error) 
{
  case EAGAIN:
  case EWOULDBLOCK:
       error = 1;
       break;
  default:
       error = 0;
}
                        ],
                        [ av_cv_eagain_ewouldblock=no ],
                        [ av_cv_eagain_ewouldblock=yes ])
        ])
    AS_IF([test "x$ac_cv_eagain_ewouldblock" = "xyes"],[
          AC_DEFINE([USE_EAGAIN], [1], [Define to true if you need to test for eagain])])
])
