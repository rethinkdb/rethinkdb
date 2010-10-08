dnl ---------------------------------------------------------------------------
dnl Macro: PROTOCOL_BINARY_TEST
dnl ---------------------------------------------------------------------------

AC_DEFUN([PROTOCOL_BINARY_TEST],
[
  AC_CACHE_CHECK([for supported struct padding], [ac_cv_supported_struct_padding], [
    AC_LANG_PUSH([C])
    save_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS -I${srcdir}"
    AC_TRY_COMPILE([
#include <inttypes.h>
#include "libmemcached/memcached/protocol_binary.h"
                   ], [
      protocol_binary_request_set request;
      int a = 1;
      switch (a) {
      case sizeof(request):
      case sizeof(request.bytes):
          break;
      default:
          a = 2;
      }
                   ],
                   [ ac_cv_supported_struct_padding=no ],
                   [ ac_cv_supported_struct_padding=yes ])
   CFLAGS="$save_CFLAGS"
   AC_LANG_POP
  ])
  AS_IF([test "x$ac_cv_supported_struct_padding" = "xno"],[
        AC_MSG_ERROR([Unsupported struct padding done by compiler.])])
])

dnl ---------------------------------------------------------------------------
dnl End Macro: PROTOCOL_BINARY_TEST
dnl ---------------------------------------------------------------------------
