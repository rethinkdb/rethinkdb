# We check two things: where the include file is for cstdint. We
# include AC_TRY_COMPILE for all the combinations we've seen in the
# wild.  We define one of HAVE_CSTDINT or HAVE_TR1_CSTDINT or 
# HAVE_BOOST_CSTDINT depending
# on location.

AC_DEFUN([PANDORA_CXX_CSTDINT],
  [AC_MSG_CHECKING(the location of cstdint)
   AC_LANG_PUSH(C++)
   save_CXXFLAGS="${CXXFLAGS}"
   CXXFLAGS="${CXX_STANDARD} ${CXXFLAGS}"
   ac_cv_cxx_cstdint=""
   for location in tr1/cstdint boost/cstdint cstdint; do
     if test -z "$ac_cv_cxx_cstdint"; then
       AC_TRY_COMPILE([#include <$location>],
                      [uint32_t t],
                      [ac_cv_cxx_cstdint="<$location>";])
     fi
   done
   AC_LANG_POP()
   CXXFLAGS="${save_CXXFLAGS}"
   if test -n "$ac_cv_cxx_cstdint"; then
      AC_MSG_RESULT([$ac_cv_cxx_cstdint])
   else
      AC_DEFINE([__STDC_CONSTANT_MACROS],[1],[Use STDC Constant Macros in C++])
      AC_DEFINE([__STDC_FORMAT_MACROS],[1],[Use STDC Format Macros in C++])
      ac_cv_cxx_cstdint="<stdint.h>"
      AC_MSG_RESULT()
      AC_MSG_WARN([Could not find a cstdint header.])
   fi
   AC_DEFINE_UNQUOTED(CSTDINT_H,$ac_cv_cxx_cstdint,
                      [the location of <cstdint>])
])
