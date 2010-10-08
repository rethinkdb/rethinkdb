dnl ---------------------------------------------------------------------------
dnl Macro: SETSOCKOPT_SANITY
dnl ---------------------------------------------------------------------------
AC_DEFUN([SETSOCKOPT_SANITY],[
  AC_CACHE_CHECK([for working SO_SNDTIMEO], [ac_cv_have_so_sndtimeo],
  AC_LANG_PUSH([C])
  AC_RUN_IFELSE([ 
    AC_LANG_PROGRAM([[
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
   ]],[[
     int sock = socket(AF_INET, SOCK_STREAM, 0);
     struct timeval waittime;
   
     waittime.tv_sec= 0;
     waittime.tv_usec= 500;
   
     if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, 
                    &waittime, (socklen_t)sizeof(struct timeval)) == -1) {
       if (errno == ENOPROTOOPT) {
         return 1;
       }
     }
     return 0;
   ]])],
   [ac_cv_have_so_sndtimeo=yes],
   [ac_cv_have_so_sndtimeo=no],
   [ac_cv_have_so_sndtimeo=yes])

   AS_IF([test "x$ac_cv_have_so_sndtimeo" = "xyes"], [
          AC_DEFINE(HAVE_SNDTIMEO, 1, [Define to 1 if you have a working SO_SNDTIMEO])])
   AC_LANG_POP
   )

  AC_CACHE_CHECK([for working SO_RCVTIMEO], [ac_cv_have_so_rcvtimeo],
  AC_LANG_PUSH([C])
  AC_RUN_IFELSE([ 
    AC_LANG_PROGRAM([[
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
   ]],[[
     int sock = socket(AF_INET, SOCK_STREAM, 0);
     struct timeval waittime;
   
     waittime.tv_sec= 0;
     waittime.tv_usec= 500;
   
     if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, 
                    &waittime, (socklen_t)sizeof(struct timeval)) == -1) {
       if (errno == ENOPROTOOPT) {
         return 1;
       }
     }
     return 0;
   ]])],
   [ac_cv_have_so_rcvtimeo=yes],
   [ac_cv_have_so_rcvtimeo=no],
   [ac_cv_have_so_rcvtimeo=yes])

   AS_IF([test "x$ac_cv_have_so_rcvtimeo" = "xyes"], [
          AC_DEFINE(HAVE_RCVTIMEO, 1, [Define to 1 if you have a working SO_RCVTIMEO])])
   AC_LANG_POP
   )
])
dnl ---------------------------------------------------------------------------
dnl End Macro: SETSOCKOPT_SANITY
dnl ---------------------------------------------------------------------------
