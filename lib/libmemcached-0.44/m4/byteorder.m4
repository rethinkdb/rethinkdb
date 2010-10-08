AC_DEFUN([DETECT_BYTEORDER],
[
    AC_REQUIRE([AC_C_BIGENDIAN])
    AC_CACHE_CHECK([for htonll], [ac_cv_have_htonll],
        [AC_TRY_LINK([
#include <sys/types.h>
#include <netinet/in.h>
#include <inttypes.h>
                        ], [
return htonll(0);
                        ],
                        [ ac_cv_have_htonll=yes ],
                        [ ac_cv_have_htonll=no ])
        ])
    AS_IF([test "x$ac_cv_have_htonll" = "xyes"],[
          AC_DEFINE([HAVE_HTONLL], [1], [Have ntohll])])

    AM_CONDITIONAL([BUILD_BYTEORDER],[test "x$ac_cv_have_htonll" = "xno"])
])
