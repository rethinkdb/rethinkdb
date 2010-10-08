dnl Copyright (C) 2010 Monty Taylor
dnl This file is free software; Monty Taylor
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

#--------------------------------------------------------------------
# Check for clock_gettime
#--------------------------------------------------------------------

AC_DEFUN([PANDORA_CLOCK_GETTIME],[
  AC_SEARCH_LIBS([clock_gettime],[rt])
  AS_IF([test "x${ac_cv_search_clock_gettime}" != "xno"],[
    AC_DEFINE([HAVE_CLOCK_GETTIME],[1],[Have a working clock_gettime function])
  ])
])
