dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([PANDORA_EXTENSIONS],[

  m4_ifdef([AC_USE_SYSTEM_EXTENSIONS],
    [AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])],
    [AC_REQUIRE([AC_GNU_SOURCE])])

])

AC_DEFUN([gl_USE_SYSTEM_EXTENSIONS],[
  AC_REQUIRE([PANDORA_EXTENSIONS])
])
