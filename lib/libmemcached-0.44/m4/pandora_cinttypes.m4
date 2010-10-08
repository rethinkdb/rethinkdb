# We check two things: where the include file is for cinttypes. We
# include AC_TRY_COMPILE for all the combinations we've seen in the
# wild.  We define one of HAVE_CINTTYPES or HAVE_TR1_CINTTYPES or 
# HAVE_BOOST_CINTTYPES depending
# on location.

AC_DEFUN([PANDORA_CXX_CINTTYPES],
  [AC_REQUIRE([PANDORA_CXX_CSTDINT])
   AC_MSG_CHECKING(the location of cinttypes)
   AC_LANG_PUSH(C++)
   save_CXXFLAGS="${CXXFLAGS}"
   CXXFLAGS="${CXX_STANDARD} ${CXXFLAGS}"
   ac_cv_cxx_cinttypes=""
   for location in tr1/cinttypes boost/cinttypes cinttypes; do
     if test -z "$ac_cv_cxx_cinttypes"; then
       AC_TRY_COMPILE([#include $ac_cv_cxx_cstdint;
                       #include <$location>],
                      [uint32_t foo= UINT32_C(1)],
                      [ac_cv_cxx_cinttypes="<$location>";])
     fi
   done
   AC_LANG_POP()
   CXXFLAGS="${save_CXXFLAGS}"
   if test -n "$ac_cv_cxx_cinttypes"; then
      AC_MSG_RESULT([$ac_cv_cxx_cinttypes])
   else
      ac_cv_cxx_cinttypes="<inttypes.h>"
      AC_MSG_RESULT()
      AC_MSG_WARN([Could not find a cinttypes header.])
   fi
   AC_DEFINE([__STDC_LIMIT_MACROS],[1],[Use STDC Limit Macros in C++])
   AC_DEFINE_UNQUOTED(CINTTYPES_H,$ac_cv_cxx_cinttypes,
                      [the location of <cinttypes>])
])
