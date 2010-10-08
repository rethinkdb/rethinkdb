dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

#--------------------------------------------------------------------
# Check for libuuid
#--------------------------------------------------------------------


AC_DEFUN([_PANDORA_SEARCH_LIBUUID],[
  AC_REQUIRE([AC_LIB_PREFIX])

  dnl Do this by hand. Need to check for uuid/uuid.h, but uuid may or may
  dnl not be a lib is weird.
  AC_CHECK_HEADERS(uuid/uuid.h)
  AC_LIB_HAVE_LINKFLAGS(uuid,,
  [
    #include <uuid/uuid.h>
  ],
  [
    uuid_t uout;
    uuid_generate(uout);
  ])

  AM_CONDITIONAL(HAVE_LIBUUID, [test "x${ac_cv_libuuid}" = "xyes"])
])

AC_DEFUN([_PANDORA_HAVE_LIBUUID],[

  AC_ARG_ENABLE([libuuid],
    [AS_HELP_STRING([--disable-libuuid],
      [Build with libuuid support @<:@default=on@:>@])],
    [ac_enable_libuuid="$enableval"],
    [ac_enable_libuuid="yes"])

  _PANDORA_SEARCH_LIBUUID
])


AC_DEFUN([PANDORA_HAVE_LIBUUID],[
  AC_REQUIRE([_PANDORA_HAVE_LIBUUID])
])

AC_DEFUN([_PANDORA_REQUIRE_LIBUUID],[
  ac_enable_libuuid="yes"
  _PANDORA_SEARCH_LIBUUID
  AS_IF([test "x$ac_cv_header_uuid_uuid_h" = "xno"],[
    AC_MSG_ERROR([Couldn't find uuid/uuid.h. On Debian this can be found in uuid-dev. On Redhat this can be found in e2fsprogs-devel.])
  ])
])

AC_DEFUN([PANDORA_REQUIRE_LIBUUID],[
  AC_REQUIRE([_PANDORA_REQUIRE_LIBUUID])
])
