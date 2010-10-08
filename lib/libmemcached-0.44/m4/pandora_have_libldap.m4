dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

#--------------------------------------------------------------------
# Check for libldap
#--------------------------------------------------------------------


AC_DEFUN([_PANDORA_SEARCH_LIBLDAP],[
  AC_REQUIRE([AC_LIB_PREFIX])

  AC_LIB_HAVE_LINKFLAGS(ldap,,
  [#include <ldap.h>],
  [
    LDAP *ldap;
    ldap_initialize(&ldap, "ldap://localhost/");
  ])
  AS_IF([test "x$ac_cv_libldap" = "xno"],
  [
    unset ac_cv_libldap
    unset HAVE_LIBLDAP
    unset LIBLDAP
    unset LIBLDAP_PREFIX
    unset LTLIBLDAP
    AC_LIB_HAVE_LINKFLAGS(ldap,,
    [#include <ldap/ldap.h>],
    [
      LDAP *ldap;
      ldap_initialize(&ldap, "ldap://localhost/");
    ])
    AS_IF([test "x$ac_cv_libldap" = "xyes"], [
      ac_cv_ldap_location="<ldap/ldap.h>"
    ])
  ],[
    ac_cv_ldap_location="<ldap.h>"
  ])

  AM_CONDITIONAL(HAVE_LIBLDAP, [test "x${ac_cv_libldap}" = "xyes"])
])

AC_DEFUN([_PANDORA_HAVE_LIBLDAP],[

  AC_ARG_ENABLE([libldap],
    [AS_HELP_STRING([--disable-libldap],
      [Build with libldap support @<:@default=on@:>@])],
    [ac_enable_libldap="$enableval"],
    [ac_enable_libldap="yes"])

  _PANDORA_SEARCH_LIBLDAP
])


AC_DEFUN([PANDORA_HAVE_LIBLDAP],[
  AC_REQUIRE([_PANDORA_HAVE_LIBLDAP])
])

AC_DEFUN([_PANDORA_REQUIRE_LIBLDAP],[
  ac_enable_libldap="yes"
  _PANDORA_SEARCH_LIBLDAP

  AS_IF([test x$ac_cv_libldap = xno],[
    AC_MSG_ERROR([libldap is required for ${PACKAGE}. On Debian this can be found in libldap2-dev. On RedHat this can be found in openldap-devel.])
  ],[
    AC_DEFINE_UNQUOTED(LDAP_HEADER,[${ac_cv_ldap_location}],
                       [Location of ldap header])
  ])
])

AC_DEFUN([PANDORA_REQUIRE_LIBLDAP],[
  AC_REQUIRE([_PANDORA_REQUIRE_LIBLDAP])
])
